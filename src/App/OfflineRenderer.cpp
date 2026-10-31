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
#include "OfflineRenderer.h"
#include "OrchestrionCommon/OfflineRendering.h"

#include <QAnimationDriver>
#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QProcess>
#include <QQuickWindow>
#include <QTemporaryFile>
#include <QTimer>
#include <QTimerEvent>
#include <private/qeventdispatcher_unix_p.h>
#include <qpa/qwindowsysteminterface.h>

#include <algorithm>
#include <cstdio>
#include <dlfcn.h>
#include <iostream>
#include <map>
#include <optional>
#include <pthread.h>
#include <time.h>

namespace dgk
{
namespace
{
using namespace std::chrono;
using Duration = QAbstractEventDispatcher::Duration;

// 16:10, like the website's frame (docs/), where the videos play
constexpr int width = 1920;
constexpr int height = 1200;
constexpr int fps = 30;

/**
 * The time of the run since prepare().
 */
nanoseconds virtualNow{0};

/**
 * Whether the main thread's clocks read the virtual time, and what they read
 * when it is 0.
 */
bool clocksVirtual = false;
pthread_t mainThread;
timespec monotonicBase;
timespec realtimeBase;

/**
 * The main thread's: Qt's, but with timers on the virtual clock, which, when
 * there is nothing else to do, moves to when the next one is due. Other
 * threads keep Qt's dispatcher and the wall clock.
 */
class VirtualTimeDispatcher : public QAbstractEventDispatcherV2
{
public:
  bool processEvents(QEventLoop::ProcessEventsFlags flags) override
  {
    m_interrupted = false;
    emit awake();
    const auto nonBlocking = flags & ~QEventLoop::WaitForMoreEvents;
    bool busy = QWindowSystemInterface::sendWindowSystemEvents(nonBlocking);
    busy |= m_inner.processEvents(nonBlocking);
    if (!(flags & QEventLoop::X11ExcludeTimers))
      busy |= fireDueTimers();
    if (busy || m_interrupted || !(flags & QEventLoop::WaitForMoreEvents))
      return busy;
    if (const auto next = nextDue())
    {
      virtualNow = *next;
      fireDueTimers();
      return true;
    }
    // No timer: only another thread can give work
    emit aboutToBlock();
    return m_inner.processEvents(flags);
  }

  void registerSocketNotifier(QSocketNotifier *notifier) override
  {
    m_inner.registerSocketNotifier(notifier);
  }
  void unregisterSocketNotifier(QSocketNotifier *notifier) override
  {
    m_inner.unregisterSocketNotifier(notifier);
  }

  void registerTimer(Qt::TimerId id, Duration interval, Qt::TimerType,
                     QObject *object) override
  {
    m_timers[id] = {object, interval, virtualNow + interval};
  }
  bool unregisterTimer(Qt::TimerId id) override
  {
    return m_timers.erase(id) > 0;
  }
  bool unregisterTimers(QObject *object) override
  {
    return std::erase_if(m_timers, [object](const auto &entry)
                         { return entry.second.object == object; }) > 0;
  }
  QList<TimerInfoV2> timersForObject(QObject *object) const override
  {
    QList<TimerInfoV2> list;
    for (const auto &[id, timer] : m_timers)
      if (timer.object == object)
        list.push_back({timer.interval, id, Qt::PreciseTimer});
    return list;
  }
  Duration remainingTime(Qt::TimerId id) const override
  {
    const auto it = m_timers.find(id);
    if (it == m_timers.end())
      return Duration::min();
    return std::max(Duration{0}, it->second.due - virtualNow);
  }

  void wakeUp() override { m_inner.wakeUp(); }
  void interrupt() override
  {
    m_interrupted = true;
    m_inner.interrupt();
  }

private:
  struct Timer
  {
    QObject *object = nullptr;
    Duration interval{0};
    nanoseconds due{0};
  };

  std::optional<nanoseconds> nextDue() const
  {
    std::optional<nanoseconds> next;
    for (const auto &[id, timer] : m_timers)
      if (!next || timer.due < *next)
        next = timer.due;
    return next;
  }

  /**
   * Each timer that is due, once, in the order they are due, then of their
   * ids. A timer may stop others, or itself.
   */
  bool fireDueTimers()
  {
    std::vector<std::pair<nanoseconds, Qt::TimerId>> due;
    for (const auto &[id, timer] : m_timers)
      if (timer.due <= virtualNow)
        due.emplace_back(timer.due, id);
    std::sort(due.begin(), due.end());
    for (const auto &[_, id] : due)
    {
      const auto it = m_timers.find(id);
      if (it == m_timers.end())
        continue;
      it->second.due = virtualNow + it->second.interval;
      QTimerEvent event{id};
      QCoreApplication::sendEvent(it->second.object, &event);
    }
    return !due.empty();
  }

  // Not installed: does the rest of the job (posted events, sockets, waking
  // up from other threads)
  QEventDispatcherUNIX m_inner;
  std::map<Qt::TimerId, Timer> m_timers;
  bool m_interrupted = false;
};

/**
 * The time Qt's animations see.
 */
class VirtualAnimationDriver : public QAnimationDriver
{
public:
  qint64 elapsed() const override
  {
    return duration_cast<milliseconds>(virtualNow).count();
  }
};

struct State
{
  QString videoPath;
  // The video without its sound, until finish()
  QString silentVideoPath;
  QTemporaryFile screenConfig;
  VirtualAnimationDriver *driver = nullptr;
  QQuickWindow *window = nullptr;
  FILE *encoder = nullptr;
  // The time of the first frame
  nanoseconds captureStart{0};
  steady_clock::time_point captureStartClock;
  qint64 frame = 0;
};

State &state()
{
  static State s;
  return s;
}

QQuickWindow *mainWindow()
{
  for (QWindow *window : QGuiApplication::topLevelWindows())
    if (auto quick = qobject_cast<QQuickWindow *>(window);
        quick && quick->isVisible())
      return quick;
  return nullptr;
}

void renderFrame()
{
  State &s = state();
  if (!s.encoder)
    return;
  s.driver->advance();
  QImage image = s.window->grabWindow();
  if (image.size() != QSize{width, height})
    image = image.scaled(width, height);
  // Byte order B, G, R, A on little-endian machines; opaque, so premultiplied
  // or not makes no difference
  if (image.format() != QImage::Format_ARGB32_Premultiplied &&
      image.format() != QImage::Format_ARGB32 &&
      image.format() != QImage::Format_RGB32)
    image = image.convertToFormat(QImage::Format_ARGB32);
  for (int y = 0; y < image.height(); ++y)
    std::fwrite(image.constScanLine(y), 4, image.width(), s.encoder);
  ++s.frame;
  const auto due = s.captureStart + nanoseconds{s.frame * 1000000000 / fps};
  QTimer::singleShot(due - virtualNow, Qt::PreciseTimer, qApp, renderFrame);
}

/**
 * A shell word.
 */
QByteArray quoted(const QString &arg)
{
  return "'" + arg.toUtf8().replace("'", "'\\''") + "'";
}
} // namespace

void OfflineRenderer::prepare(const QString &videoPath)
{
  State &s = state();
  s.videoPath = QFileInfo(videoPath).absoluteFilePath();
  s.silentVideoPath = s.videoPath + ".silent.mp4";
  offlineRendering.audioPath = (s.videoPath + ".f32").toStdString();

  // Drawn offscreen, at the video's size
  s.screenConfig.setFileTemplate(QDir::tempPath() +
                                 "/orchestrion-screen-XXXXXX.json");
  if (s.screenConfig.open())
  {
    s.screenConfig.write(QStringLiteral(R"({"screens": [{"name": "Video",
      "x": 0, "y": 0, "width": %1, "height": %2,
      "logicalDpi": 96, "logicalBaseDpi": 96, "dpr": 1}]})")
                             .arg(width)
                             .arg(height)
                             .toUtf8());
    s.screenConfig.close();
  }
  qputenv("QT_QPA_PLATFORM",
          ("offscreen:configfile=" + s.screenConfig.fileName()).toUtf8());
  qunsetenv("QT_QPA_PLATFORMTHEME");
  // Frames rendered when grabbed, on the main thread
  qputenv("QSG_RENDER_LOOP", "basic");

  mainThread = pthread_self();
  clock_gettime(CLOCK_MONOTONIC, &monotonicBase);
  clock_gettime(CLOCK_REALTIME, &realtimeBase);
  clocksVirtual = true;
  QCoreApplication::setEventDispatcher(new VirtualTimeDispatcher);
}

bool OfflineRenderer::active() { return clocksVirtual; }

void OfflineRenderer::start()
{
  if (!active())
    return;
  State &s = state();
  s.driver = new VirtualAnimationDriver;
  s.driver->install();
  // In the middle of the screen, as on a display just started, rather than in
  // the corner, where it would hover over what is there (scripts point with
  // the demo cursor)
  QCursor::setPos(width / 2, height / 2);
}

void OfflineRenderer::startCapture()
{
  State &s = state();
  if (!active() || s.encoder)
    return;
  s.window = mainWindow();
  if (!s.window)
  {
    std::cout << "[render] no window to render" << std::endl;
    return;
  }
  QFile::remove(s.silentVideoPath);
  const QByteArray command =
      "ffmpeg -loglevel error -y -f rawvideo -pix_fmt bgra -s " +
      QByteArray::number(width) + "x" + QByteArray::number(height) + " -r " +
      QByteArray::number(fps) +
      " -i - -c:v libx264 -preset veryfast -pix_fmt yuv420p " +
      quoted(s.silentVideoPath);
  s.encoder = popen(command.constData(), "w");
  if (!s.encoder)
  {
    std::cout << "[render] couldn't run ffmpeg" << std::endl;
    return;
  }
  s.captureStart = virtualNow;
  s.captureStartClock = steady_clock::now();
  s.frame = 0;
  renderFrame();
}

void OfflineRenderer::finish()
{
  State &s = state();
  if (!active())
    return;
  if (!s.encoder)
  {
    std::cout << "[render] nothing was rendered" << std::endl;
    return;
  }
  pclose(s.encoder);
  s.encoder = nullptr;

  // The sound, from the first frame's time
  QStringList args{"-loglevel", "error", "-y", "-i", s.silentVideoPath};
  const QString audioPath = QString::fromStdString(offlineRendering.audioPath);
  const bool hasAudio =
      offlineRendering.sampleRate > 0 && QFileInfo(audioPath).size() > 0;
  if (hasAudio)
  {
    const double offset =
        duration<double>(s.captureStartClock - offlineRendering.audioStart)
            .count();
    args << "-ss" << QString::number(std::max(0.0, offset), 'f', 6) << "-f"
         << "f32le" << "-ar" << QString::number(offlineRendering.sampleRate)
         << "-ac" << QString::number(offlineRendering.channelCount) << "-i"
         << audioPath << "-map" << "0:v" << "-map" << "1:a" << "-c:a" << "aac"
         << "-b:a" << "160k" << "-shortest";
  }
  else
    std::cout << "[render] no sound was rendered" << std::endl;
  // The index first, so that a browser can start playing it at once
  args << "-c:v" << "copy" << "-movflags" << "+faststart" << s.videoPath;
  if (QProcess::execute("ffmpeg", args) != 0)
    std::cout << "[render] ffmpeg couldn't write " << s.videoPath.toStdString()
              << std::endl;
  QFile::remove(s.silentVideoPath);
  QFile::remove(audioPath);
  std::cout << "[render] " << s.frame
            << " frames: " << s.videoPath.toStdString() << std::endl;
}
} // namespace dgk

/**
 * The C library's, which this definition shadows for the whole process (it
 * is what Qt's QElapsedTimer, QDateTime and std::chrono's clocks read): while
 * rendering, the main thread reads the virtual time.
 */
extern "C" int clock_gettime(clockid_t clock, timespec *time)
{
  using Real = int (*)(clockid_t, timespec *);
  static const auto real =
      reinterpret_cast<Real>(dlsym(RTLD_NEXT, "clock_gettime"));
  const int result = real(clock, time);
  if (result != 0 || !dgk::clocksVirtual ||
      !pthread_equal(pthread_self(), dgk::mainThread))
    return result;
  const timespec *base = nullptr;
  switch (clock)
  {
  case CLOCK_MONOTONIC:
  case CLOCK_MONOTONIC_RAW:
  case CLOCK_MONOTONIC_COARSE:
  case CLOCK_BOOTTIME:
    base = &dgk::monotonicBase;
    break;
  case CLOCK_REALTIME:
  case CLOCK_REALTIME_COARSE:
    base = &dgk::realtimeBase;
    break;
  default:
    return result;
  }
  const long long ns =
      base->tv_sec * 1000000000LL + base->tv_nsec + dgk::virtualNow.count();
  time->tv_sec = ns / 1000000000LL;
  time->tv_nsec = ns % 1000000000LL;
  return result;
}
