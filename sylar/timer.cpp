#include "timer.h"
#include "sylar/thread.h"
#include "util.h"
#include <functional>
#include <limits>
#include <memory>
#include <vector>

namespace sylar {
bool Timer::Comparator::operator()( const Timer::SPtr& lhs, const Timer::SPtr& rhs ) const
{
  if ( !lhs && !rhs ) {
    return false;
  }

  if ( !lhs ) {
    return true;
  }

  if ( !rhs ) {
    return false;
  }

  if ( lhs->m_next < rhs->m_next ) {
    return true;
  }

  if ( rhs->m_next < lhs->m_next ) {
    return false;
  }

  return lhs.get() < rhs.get();
}

Timer::Timer( std::uint64_t ms, std::function<void()> cb, bool recurring, TimerManager* manager )
  : m_recurring( recurring ), m_ms( ms ), m_cb( cb ), m_manager( manager )
{
  m_next = sylar::GetCurrentMS() + m_ms;
}

Timer::Timer( std::uint64_t next ) : m_next( next ) {}

bool Timer::cancel()
{
  TimerManager::RWMutexType::WriteLock lock { m_manager->m_mutex };
  if ( m_cb ) {
    m_cb = nullptr;
    auto it = m_manager->m_timers.find( shared_from_this() );
    m_manager->m_timers.erase( it );
    return true;
  }
  return false;
}

bool Timer::refresh()
{
  TimerManager::RWMutexType::WriteLock lock { m_manager->m_mutex };
  if ( !m_cb ) {
    return false;
  }

  auto it = m_manager->m_timers.find( shared_from_this() );
  if ( it == m_manager->m_timers.end() ) {
    return false;
  }

  m_manager->m_timers.erase( it );
  m_next = sylar::GetCurrentMS() + m_ms;
  m_manager->m_timers.insert( shared_from_this() );
  return true;
}

bool Timer::reset( std::uint64_t ms, bool from_now )
{
  if ( m_ms == ms && !from_now ) {
    return true;
  }

  TimerManager::RWMutexType::WriteLock lock { m_manager->m_mutex };
  if ( !m_cb ) {
    return false;
  }

  auto it = m_manager->m_timers.find( shared_from_this() );
  if ( it == m_manager->m_timers.end() ) {
    return false;
  }

  m_manager->m_timers.erase( it );
  std::uint64_t start { 0 };
  if ( from_now ) {
    start = sylar::GetCurrentMS();
  } else {
    start = m_next - m_ms;
  }

  m_ms = ms;
  m_next = start + m_ms;
  m_manager->addTimer( shared_from_this(), lock );
  return true;
}

TimerManager::TimerManager()
{
  m_previouseTime = sylar::GetCurrentMS();
}

TimerManager::~TimerManager() {}

Timer::SPtr TimerManager::addTimer( std::uint64_t ms, std::function<void()> cb, bool recurring )
{
  Timer::SPtr timer { std::make_shared<Timer>( ms, cb, recurring, this ) };
  RWMutexType::WriteLock lock { m_mutex };
  addTimer( timer, lock );
  return timer;
}

static void OnTimer( std::weak_ptr<void> weak_cond, std::function<void()> cb )
{
  std::shared_ptr<void> tmp = weak_cond.lock();
  if ( tmp ) {
    cb();
  }
}

Timer::SPtr TimerManager::addConditionTimer( std::uint64_t ms,
                                             std::function<void()> cb,
                                             std::weak_ptr<void> weak_cond,
                                             bool recurring )
{
  return addTimer( ms, std::bind( &OnTimer, weak_cond, cb ), recurring );
}

std::uint64_t TimerManager::getNextTimer()
{
  RWMutexType::ReadLock lock { m_mutex };
  m_tickled = false;
  if ( m_timers.empty() ) {
    return std::numeric_limits<std::uint64_t>::max();
  }

  const Timer::SPtr& next = *m_timers.begin();
  std::uint64_t now_ms { sylar::GetCurrentMS() };
  if ( now_ms >= next->m_next ) {
    return 0;
  } else {
    return next->m_next - now_ms;
  }
}

void TimerManager::listExpiredCb( std::vector<std::function<void()>>& cbs )
{
  std::uint64_t now_ms { sylar::GetCurrentMS() };
  std::vector<Timer::SPtr> expired;
  {
    RWMutexType::ReadLock lock { m_mutex };
    if ( m_timers.empty() ) {
      return;
    }
  }

  RWMutexType::WriteLock lock { m_mutex };
  bool rollover { detectClockRollover( now_ms ) };
  if ( !rollover && ( ( *m_timers.begin() )->m_next > now_ms ) ) {
    return;
  }

  Timer::SPtr now_timer { std::make_shared<Timer>( now_ms ) };
  auto it = rollover ? m_timers.end() : m_timers.lower_bound( now_timer );
  while ( it != m_timers.end() && ( *it )->m_next == now_ms ) {
    ++it;
  }

  expired.insert( expired.begin(), m_timers.begin(), it );
  m_timers.erase( m_timers.begin(), it );
  cbs.reserve( expired.size() );

  for ( auto& timer : expired ) {
    cbs.push_back( timer->m_cb );
    if ( timer->m_recurring ) {
      timer->m_next = now_ms + timer->m_ms;
      m_timers.insert( timer );
    } else {
      timer->m_cb = nullptr;
    }
  }
}

void TimerManager::addTimer( Timer::SPtr val, RWMutexType::WriteLock& lock )
{
  auto it = m_timers.insert( val ).first;
  bool at_front = ( it == m_timers.begin() ) && !m_tickled;
  if ( at_front ) {
    m_tickled = true;
  }
  lock.unlock();

  if ( at_front ) {
    onTimerInsertedAtFront();
  }
}

bool TimerManager::detectClockRollover( std::uint64_t now_ms )
{
  bool rollover { false };
  if ( now_ms < m_previouseTime && now_ms < ( m_previouseTime - 60 * 60 * 1000 ) ) {
    rollover = true;
  }

  m_previouseTime = now_ms;
  return rollover;
}

bool TimerManager::hasTimer()
{
  RWMutex::ReadLock lock { m_mutex };
  return !m_timers.empty();
}

}