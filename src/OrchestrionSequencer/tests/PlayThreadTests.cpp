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
#include <gtest/gtest.h>

#include "internal/PlayThread.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <atomic>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace dgk
{
namespace
{
using namespace std::chrono_literals;
using Clock = PlayThread::Clock;

/** What the tasks recorded, from whichever thread they ran on. */
struct Log
{
  std::mutex mutex;
  std::vector<int> order;
  void Add(int value)
  {
    std::scoped_lock lock{mutex};
    order.push_back(value);
  }
  std::vector<int> Get()
  {
    std::scoped_lock lock{mutex};
    return order;
  }
};

/** A QCoreApplication for the main-thread variant, if there is none yet. */
void EnsureApplication()
{
  if (QCoreApplication::instance())
    return;
  static int argc = 1;
  static char name[] = "playthread_tests";
  static char *argv[] = {name, nullptr};
  static QCoreApplication app{argc, argv};
}

/** Runs the main thread's event loop until `done` or a second has passed. */
void RunMainLoopUntil(const std::function<bool()> &done)
{
  const auto end = Clock::now() + 1s;
  while (!done() && Clock::now() < end)
    QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
}
} // namespace

TEST(PlayThread, PostsRunInOrderOnTheThread)
{
  Log log;
  std::atomic<bool> allOnThread{true};
  {
    PlayThread thread;
    EXPECT_FALSE(thread.IsCurrent());
    for (int i = 0; i < 100; ++i)
      thread.Post(
          [&, i]
          {
            if (!thread.IsCurrent())
              allOnThread = false;
            log.Add(i);
          });
    thread.Wait([] {});
  }
  const auto order = log.Get();
  ASSERT_EQ(order.size(), 100u);
  for (int i = 0; i < 100; ++i)
    EXPECT_EQ(order[i], i);
  EXPECT_TRUE(allOnThread);
}

TEST(PlayThread, TimedTasksRunWhenDueInTimeOrder)
{
  Log log;
  PlayThread thread;
  const auto start = Clock::now();
  std::atomic<long long> earliestRunUs{0};
  // Posted latest first: they must run soonest first, none early.
  for (int i = 3; i >= 1; --i)
    thread.PostAt(start + i * 20ms,
                  [&, i, start]
                  {
                    const auto late = Clock::now() - (start + i * 20ms);
                    if (late < 0ms)
                      earliestRunUs =
                          std::chrono::duration_cast<std::chrono::microseconds>(
                              late)
                              .count();
                    log.Add(i);
                  });
  std::this_thread::sleep_for(100ms);
  thread.Wait([] {});
  EXPECT_EQ(log.Get(), (std::vector<int>{1, 2, 3}));
  EXPECT_EQ(earliestRunUs.load(), 0);
}

TEST(PlayThread, SameTimeRunsInPostingOrder)
{
  Log log;
  PlayThread thread;
  const auto due = Clock::now() + 10ms;
  for (int i = 0; i < 10; ++i)
    thread.PostAt(due, [&, i] { log.Add(i); });
  std::this_thread::sleep_for(50ms);
  thread.Wait([] {});
  EXPECT_EQ(log.Get(), (std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}));
}

TEST(PlayThread, PostIsNotHeldBehindALaterTimedTask)
{
  Log log;
  PlayThread thread;
  thread.PostAt(Clock::now() + 1h, [&] { log.Add(1); });
  thread.Post([&] { log.Add(2); });
  thread.Wait([] {});
  EXPECT_EQ(log.Get(), (std::vector<int>{2}));
}

TEST(PlayThread, AnEarlierTimedTaskPostedLaterIsNotHeldBehind)
{
  Log log;
  PlayThread thread;
  thread.PostAt(Clock::now() + 1h, [&] { log.Add(1); });
  // The thread is waiting for the hour to pass: this must wake it.
  thread.PostAt(Clock::now() + 10ms, [&] { log.Add(2); });
  std::this_thread::sleep_for(60ms);
  EXPECT_EQ(log.Get(), (std::vector<int>{2}));
}

TEST(PlayThread, WaitFromTheThreadRunsInline)
{
  Log log;
  PlayThread thread;
  thread.Wait(
      [&]
      {
        log.Add(1);
        // Would deadlock if it queued.
        thread.Wait([&] { log.Add(2); });
        log.Add(3);
      });
  EXPECT_EQ(log.Get(), (std::vector<int>{1, 2, 3}));
}

TEST(PlayThread, PendingTasksAreDroppedWithTheThread)
{
  auto ran = std::make_shared<std::atomic<bool>>(false);
  std::weak_ptr<std::atomic<bool>> held;
  {
    PlayThread thread;
    held = ran;
    thread.PostAt(Clock::now() + 1h, [ran] { *ran = true; });
    ran.reset();
  }
  // Not run, and destroyed: nothing holds what it captured.
  EXPECT_TRUE(held.expired());
}

TEST(PlayThread, OnMainThreadRunsThereInOrder)
{
  EnsureApplication();
  Log log;
  bool allOnMain = true;
  PlayThread thread{/*onMainThread=*/true};
  EXPECT_TRUE(thread.IsCurrent());
  const auto start = Clock::now();
  thread.PostAt(start + 30ms, [&] { log.Add(4); });
  thread.PostAt(start + 10ms, [&] { log.Add(3); });
  thread.Post([&] { log.Add(1); });
  // From another thread, as a MIDI thread would.
  std::thread{[&] { thread.Post([&] { log.Add(2); }); }}.join();
  thread.Post([&] { allOnMain = allOnMain && thread.IsCurrent(); });
  RunMainLoopUntil([&] { return log.Get().size() == 4; });
  EXPECT_EQ(log.Get(), (std::vector<int>{1, 2, 3, 4}));
  EXPECT_TRUE(allOnMain);
  EXPECT_GE(Clock::now() - start, 30ms);
}

TEST(PlayThread, OnMainThreadWaitFromAnotherThread)
{
  EnsureApplication();
  PlayThread thread{/*onMainThread=*/true};
  std::atomic<bool> ranOnMain{false};
  std::atomic<bool> returned{false};
  std::thread other{[&]
                    {
                      thread.Wait([&] { ranOnMain = thread.IsCurrent(); });
                      returned = true;
                    }};
  RunMainLoopUntil([&] { return returned.load(); });
  other.join();
  EXPECT_TRUE(ranOnMain);
}
} // namespace dgk
