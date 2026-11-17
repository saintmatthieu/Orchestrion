/*
 * This file is part of Orchestrion.
 *
 * Copyright (C) 2026 Matthieu Hodgkinson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once

// Included last by its users: <windows.h>'s macros break headers after it.
#include <QtGlobal>
#include <log.h>

#if defined(Q_OS_WIN)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(Q_OS_MACOS)
#include <pthread.h>
#include <pthread/qos.h>
#elif defined(Q_OS_LINUX)
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusVariant>
#include <algorithm>
#include <pthread.h>
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

namespace dgk
{
#ifdef Q_OS_LINUX
namespace timingThread
{
/**
 * Above every ordinary thread, well below the audio server's (PipeWire's
 * data loop runs at 88): its output still comes first.
 */
constexpr int realTimePriority = 10;

/**
 * Asks RealtimeKit, over the system bus, to make the calling thread a
 * real-time one: what a process without the permission to do so itself
 * resorts to, as PipeWire's clients do. Returns the priority given, 0 if
 * none was.
 */
inline int AskRealtimeKit()
{
  const QString service = QStringLiteral("org.freedesktop.RealtimeKit1");
  const QString path = QStringLiteral("/org/freedesktop/RealtimeKit1");
  constexpr int timeoutMs = 1000;
  QDBusConnection bus = QDBusConnection::systemBus();
  if (!bus.isConnected())
    return 0;
  const auto property = [&](const char *name) -> qlonglong
  {
    QDBusMessage get = QDBusMessage::createMethodCall(
        service, path, QStringLiteral("org.freedesktop.DBus.Properties"),
        QStringLiteral("Get"));
    get << service << QString::fromLatin1(name);
    const QDBusMessage reply = bus.call(get, QDBus::Block, timeoutMs);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().empty())
      return 0;
    return reply.arguments()
        .first()
        .value<QDBusVariant>()
        .variant()
        .toLongLong();
  };

  // It only serves a process that limits how long its real-time threads may
  // run without blocking: one gone wild is then stopped by the kernel
  // rather than left to starve the others.
  const qlonglong maxRunUs = property("RTTimeUSecMax");
  const int priority =
      std::min<qlonglong>(realTimePriority, property("MaxRealtimePriority"));
  if (maxRunUs <= 0 || priority <= 0)
    return 0;
  rlimit limit{};
  if (getrlimit(RLIMIT_RTTIME, &limit) != 0)
    return 0;
  if (limit.rlim_cur == RLIM_INFINITY ||
      limit.rlim_cur > static_cast<rlim_t>(maxRunUs))
  {
    limit.rlim_cur = static_cast<rlim_t>(maxRunUs);
    if (limit.rlim_max != RLIM_INFINITY && limit.rlim_max < limit.rlim_cur)
      limit.rlim_cur = limit.rlim_max;
    if (setrlimit(RLIMIT_RTTIME, &limit) != 0)
      return 0;
  }

  QDBusMessage make = QDBusMessage::createMethodCall(
      service, path, service, QStringLiteral("MakeThreadRealtime"));
  make << static_cast<qulonglong>(syscall(SYS_gettid))
       << static_cast<quint32>(priority);
  return bus.call(make, QDBus::Block, timeoutMs).type() ==
                 QDBusMessage::ReplyMessage
             ? priority
             : 0;
}
} // namespace timingThread
#endif

/**
 * For a thread that times what is played (see
 * OrchestrionSequencer/threading.md), called by it first thing: names it
 * (15 characters at most, for Linux; not on Windows) and has it run before the
 * ordinary threads when there is contention for a core. It only does short work
 * between waits; one whose work grows with the score (the performance
 * follower's) isn't to call this.
 *
 * - Linux: SCHED_FIFO at a low real-time priority, set directly if the user
 *   may (RLIMIT_RTPRIO, as members of the audio group often do), else asked
 *   of RealtimeKit; an ordinary thread if neither. (`nice` can't be lowered
 *   without privilege.)
 * - Windows: above normal.
 * - macOS: the user-interactive quality of service.
 */
inline void SetUpTimingThread(const char *name)
{
// (Braces throughout: LOGI and LOGW are `if` statements, which an `else`
// following them would attach to.)
#if defined(Q_OS_WIN)
  if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL))
  {
    LOGW() << name << ": priority not raised";
  }
#elif defined(Q_OS_MACOS)
  pthread_setname_np(name);
  if (pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0) != 0)
  {
    LOGW() << name << ": priority not raised";
  }
#elif defined(Q_OS_LINUX)
  pthread_setname_np(pthread_self(), name);
  sched_param param{};
  param.sched_priority = timingThread::realTimePriority;
  // Not inherited by a process it would start
  if (sched_setscheduler(0, SCHED_FIFO | SCHED_RESET_ON_FORK, &param) == 0)
  {
    LOGI() << name << ": real-time priority " << timingThread::realTimePriority;
  }
  else if (const int priority = timingThread::AskRealtimeKit())
  {
    LOGI() << name << ": real-time priority " << priority
           << ", from RealtimeKit";
  }
  else
  {
    LOGI() << name
           << ": ordinary priority (no real-time permission, no RealtimeKit)";
  }
#else
  (void)name;
#endif
}
} // namespace dgk
