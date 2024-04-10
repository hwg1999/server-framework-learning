#pragma once

#include "sylar/scheduler.h"
#include "sylar/timer.h"

namespace sylar {

class IOManager
  : public Scheduler
  , public TimerManager
{
public:
  using SPtr = std::shared_ptr<IOManager>;
  using RWMutexType = RWMutex;

  enum Event
  {
    NONE = 0x0,
    READ = 0x1,  // EPOLLIN
    WRITE = 0x4, // EPOLLOUT
  };

private:
  struct FdContext
  {
    using MutexType = Mutex;
    struct EventContext
    {
      Scheduler* scheduler { nullptr };
      Fiber::SPtr fiber;
      std::function<void()> cb;
    };

    EventContext& getContext( Event event );
    void resetContext( EventContext& ctx );
    void triggerEvent( Event event );

    EventContext read;
    EventContext write;
    int fd { 0 };
    Event events { NONE };
    MutexType mutex;
  };

public:
  IOManager( std::size_t threads = 1, bool user_caller = true, const std::string& name = "" );
  ~IOManager();

  int addEvent( int fd, Event event, std::function<void()> cb = nullptr );
  bool delEvent( int fd, Event event );
  bool cancelEvent( int fd, Event event );

  bool cancelAll( int fd );

  static IOManager* GetThis();

protected:
  void tickle() override;
  bool stopping() override;
  void idle() override;
  void onTimerInsertedAtFront() override;

  void contextResize( std::size_t size );
  bool stopping( std::uint64_t& timeout );

private:
  int m_epfd { 0 };
  int m_tickleFds[2];

  std::atomic<std::size_t> m_pendingEventCount { 0 };
  RWMutexType m_mutex;
  std::vector<FdContext*> m_fdContexts;
};

}