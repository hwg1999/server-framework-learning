#pragma once

#include "thread.h"
#include <functional>
#include <memory>
#include <set>
#include <vector>

namespace sylar {

class TimerManager;
class Timer : public std::enable_shared_from_this<Timer>
{
  friend class TimerManager;

public:
  using SPtr = std::shared_ptr<Timer>;
  bool cancel();
  bool refresh();
  bool reset( std::uint64_t ms, bool from_now );

private:
  Timer( std::uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager );
  Timer( std::uint64_t next );

private:
  bool m_recurring { false };
  std::uint64_t m_ms { 0 };
  std::uint64_t m_next { 0 };
  std::function<void()> m_cb;
  TimerManager* m_manager { nullptr };

private:
  struct Comparator
  {
    bool operator()( const Timer::SPtr& lhs, const Timer::SPtr& rhs ) const;
  };
};

class TimerManager
{
  friend class Timer;

public:
  using RWMutexType = RWMutex;

  TimerManager();
  virtual ~TimerManager();

  Timer::SPtr addTimer( std::uint64_t ms, std::function<void()> cb, bool recurring = false );
  Timer::SPtr addConditionTimer( std::uint64_t ms,
                                 std::function<void()> cb,
                                 std::weak_ptr<void> weak_cond,
                                 bool recurring = false );
  std::uint64_t getNextTimer();
  void listExpiredCb( std::vector<std::function<void()>>& cbs );
  bool hasTimer();

protected:
  virtual void onTimerInsertedAtFront() = 0;
  void addTimer( Timer::SPtr val, RWMutexType::WriteLock& lock );

private:
  bool detectClockRollover( std::uint64_t now_ms );

private:
  RWMutexType m_mutex;
  std::set<Timer::SPtr, Timer::Comparator> m_timers;
  bool m_tickled { false };
  std::uint64_t m_previouseTime { 0 };
};

}