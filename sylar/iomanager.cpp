#include "iomanager.h"
#include "log.h"
#include "macro.h"
#include "sylar/fiber.h"
#include "sylar/scheduler.h"

#include <cassert>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <functional>
#include <memory>
#include <sys/epoll.h>
#include <unistd.h>

namespace sylar {

static sylar::Logger::SPtr g_logger { SYLAR_LOG_NAME( "system" ) };

IOManager::FdContext::EventContext& IOManager::FdContext::getContext( IOManager::Event event )
{
  switch ( event ) {
    case IOManager::READ:
      return read;
    case IOManager::WRITE:
      return write;
    default:
      SYLAR_ASSERT2( false, "getContext" );
  }
}

void IOManager::FdContext::resetContext( EventContext& ctx )
{
  ctx.scheduler = nullptr;
  ctx.fiber.reset();
  ctx.cb = nullptr;
}

void IOManager::FdContext::triggerEvent( IOManager::Event event )
{
  SYLAR_ASSERT( events & event );
  events = (Event)( events & ~event );
  EventContext& ctx = getContext( event );
  if ( ctx.cb ) {
    ctx.scheduler->schedule( &ctx.cb );
  } else {
    ctx.scheduler->schedule( &ctx.fiber );
  }
  ctx.scheduler = nullptr;
}

IOManager::IOManager( std::size_t threads, bool user_caller, const std::string& name )
  : Scheduler( threads, user_caller, name ), m_epfd( epoll_create( 5000 ) )
{
  SYLAR_ASSERT( m_epfd > 0 );

  int ret = pipe( m_tickleFds );
  SYLAR_ASSERT( !ret );

  epoll_event event;
  std::memset( &event, 0, sizeof( epoll_event ) );
  event.events = EPOLLIN | EPOLLET;
  event.data.fd = m_tickleFds[0];

  ret = fcntl( m_tickleFds[0], F_SETFL, O_NONBLOCK );
  SYLAR_ASSERT( !ret );

  ret = epoll_ctl( m_epfd, EPOLL_CTL_ADD, m_tickleFds[0], &event );
  SYLAR_ASSERT( !ret );

  contextResize( 32 );

  start();
}

IOManager::~IOManager()
{
  stop();
  close( m_epfd );
  close( m_tickleFds[0] );
  close( m_tickleFds[1] );

  for ( std::size_t i { 0 }; i < m_fdContexts.size(); ++i ) {
    if ( m_fdContexts[i] ) {
      delete m_fdContexts[i];
    }
  }
}

void IOManager::contextResize( std::size_t size )
{
  m_fdContexts.resize( size );

  for ( std::size_t i { 0 }; i < m_fdContexts.size(); ++i ) {
    if ( !m_fdContexts[i] ) {
      m_fdContexts[i] = new FdContext;
      m_fdContexts[i]->fd = i;
    }
  }
}

int IOManager::addEvent( int fd, Event event, std::function<void()> cb )
{
  FdContext* fd_ctx { nullptr };
  RWMutexType::ReadLock lock { m_mutex };
  if ( static_cast<int>( m_fdContexts.size() ) > fd ) {
    fd_ctx = m_fdContexts[fd];
    lock.unlock();
  } else {
    lock.unlock();
    RWMutexType::WriteLock writeLock { m_mutex };
    contextResize( fd * 1.5 );
    fd_ctx = m_fdContexts[fd];
  }

  FdContext::MutexType::Lock lock2 { fd_ctx->mutex };
  if ( fd_ctx->events & event ) {
    SYLAR_LOG_ERROR( g_logger ) << "addEvent assert fd = " << fd << " event = " << event
                                << "fd_ctx.event = " << fd_ctx->events;
    SYLAR_ASSERT( !( fd_ctx->events & event ) );
  }

  int op { fd_ctx->events ? EPOLL_CTL_MOD : EPOLL_CTL_ADD };
  epoll_event epevent;
  epevent.events = EPOLLET | fd_ctx->events | event;
  epevent.data.ptr = fd_ctx;

  int ret = epoll_ctl( m_epfd, op, fd, &epevent );
  if ( ret ) {
    SYLAR_LOG_ERROR( g_logger ) << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << "," << epevent.events
                                << "):" << ret << " (" << errno << ") (" << strerror( errno ) << ")";
    return -1;
  }

  ++m_pendingEventCount;
  fd_ctx->events = (Event)( fd_ctx->events | event );
  FdContext::EventContext& event_ctx { fd_ctx->getContext( event ) };
  SYLAR_ASSERT( !event_ctx.scheduler && !event_ctx.fiber && !event_ctx.cb );

  event_ctx.scheduler = Scheduler::GetThis();
  if ( cb ) {
    event_ctx.cb.swap( cb );
  } else {
    event_ctx.fiber = Fiber::GetThis();
    SYLAR_ASSERT( event_ctx.fiber->getState() == Fiber::EXEC );
  }

  return 0;
}

bool IOManager::delEvent( int fd, Event event )
{
  RWMutexType::ReadLock lock { m_mutex };
  if ( static_cast<int>( m_fdContexts.size() ) < fd ) {
    return false;
  }
  FdContext* fd_ctx { m_fdContexts[fd] };
  lock.unlock();

  FdContext::MutexType::Lock lock2 { fd_ctx->mutex };
  if ( !( fd_ctx->events & event ) ) {
    return false;
  }

  Event new_events { (Event)( fd_ctx->events & ~event ) };
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int ret { epoll_ctl( m_epfd, op, fd, &epevent ) };
  if ( ret ) {
    SYLAR_LOG_ERROR( g_logger ) << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << "," << epevent.events
                                << "):" << ret << " (" << errno << ") (" << strerror( errno ) << ")";
    return false;
  }

  --m_pendingEventCount;
  fd_ctx->events = new_events;
  FdContext::EventContext& event_ctx = fd_ctx->getContext( event );
  fd_ctx->resetContext( event_ctx );
  return true;
}

bool IOManager::cancelEvent( int fd, Event event )
{
  RWMutexType::ReadLock lock { m_mutex };
  if ( static_cast<int>( m_fdContexts.size() ) <= fd ) {
    return false;
  }
  FdContext* fd_ctx { m_fdContexts[fd] };
  lock.unlock();

  FdContext::MutexType::Lock lock2 { fd_ctx->mutex };
  if ( !( fd_ctx->events & event ) ) {
    return false;
  }

  Event new_events { (Event)( fd_ctx->events & ~event ) };
  int op = new_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = EPOLLET | new_events;
  epevent.data.ptr = fd_ctx;

  int ret = epoll_ctl( m_epfd, op, fd, &epevent );
  if ( ret ) {
    SYLAR_LOG_ERROR( g_logger ) << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << "," << epevent.events
                                << "):" << ret << " (" << errno << ") (" << strerror( errno ) << ")";
    return false;
  }

  fd_ctx->triggerEvent( event );
  --m_pendingEventCount;
  return true;
}

bool IOManager::cancelAll( int fd )
{
  RWMutexType::ReadLock lock { m_mutex };
  if ( (int)m_fdContexts.size() <= fd ) {
    return false;
  }
  FdContext* fd_ctx { m_fdContexts[fd] };
  lock.unlock();

  FdContext::MutexType::Lock lock2 { fd_ctx->mutex };
  if ( !fd_ctx->events ) {
    return false;
  }

  int op = EPOLL_CTL_DEL;
  epoll_event epevent;
  epevent.events = 0;
  epevent.data.ptr = fd_ctx;

  int ret = epoll_ctl( m_epfd, op, fd, &epevent );
  if ( ret ) {
    SYLAR_LOG_ERROR( g_logger ) << "epoll_ctl(" << m_epfd << ", " << op << "," << fd << "," << epevent.events
                                << "):" << ret << " (" << errno << ") (" << strerror( errno ) << ")";
    return false;
  }

  if ( fd_ctx->events & READ ) {
    fd_ctx->triggerEvent( READ );
    --m_pendingEventCount;
  }

  if ( fd_ctx->events & WRITE ) {
    fd_ctx->triggerEvent( WRITE );
    --m_pendingEventCount;
  }

  SYLAR_ASSERT( fd_ctx->events == 0 );

  return true;
}

IOManager* IOManager::GetThis()
{
  return dynamic_cast<IOManager*>( Scheduler::GetThis() );
}

void IOManager::tickle()
{
  if ( hasIdleThreads() ) {
    return;
  }
  int ret = write( m_tickleFds[1], "T", 1 );
  SYLAR_ASSERT( ret == 1 );
}

bool IOManager::stopping()
{
  return Scheduler::stopping() && m_pendingEventCount == 0;
}

void IOManager::idle()
{
  epoll_event* events = new epoll_event[64] {};
  std::shared_ptr<epoll_event> shared_events { events, []( epoll_event* ptr ) { delete[] ptr; } };

  while ( true ) {
    if ( stopping() ) {
      SYLAR_LOG_INFO( g_logger ) << "name = " << getName() << " idle stopping exit";
      break;
    }

    int ret { 0 };
    do {
      static constexpr const int MAX_TIMEOUT { 5000 };
      ret = epoll_wait( m_epfd, events, 64, MAX_TIMEOUT );
      if ( !( ret < 0 && errno == EINTR ) ) {
        break;
      }
    } while ( true );

    for ( int i = 0; i < ret; ++i ) {
      epoll_event& event = events[i];
      if ( event.data.fd == m_tickleFds[0] ) {
        std::uint8_t dummy;
        while ( read( m_tickleFds[0], &dummy, 1 ) == 1 )
          ;
        continue;
      }

      FdContext* fd_ctx = (FdContext*)event.data.ptr;
      FdContext::MutexType::Lock lock { fd_ctx->mutex };
      if ( event.events & ( EPOLLERR | EPOLLHUP ) ) {
        event.events |= EPOLLIN | EPOLLOUT;
      }

      int real_events { NONE };
      if ( event.events & EPOLLIN ) {
        real_events |= READ;
      }

      if ( event.events & EPOLLOUT ) {
        real_events |= WRITE;
      }

      if ( ( fd_ctx->events & real_events ) == NONE ) {
        continue;
      }

      int left_events { fd_ctx->events & ~real_events };
      int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = EPOLLET | left_events;

      int ret2 { epoll_ctl( m_epfd, op, fd_ctx->fd, &event ) };
      if ( ret2 ) {
        SYLAR_LOG_ERROR( g_logger ) << "epoll_ctl(" << m_epfd << ", " << op << "," << fd_ctx->fd << ","
                                    << event.events << "):" << ret2 << " (" << errno << ") (" << strerror( errno )
                                    << ")";
        continue;
      }

      if ( real_events & READ ) {
        fd_ctx->triggerEvent( READ );
        --m_pendingEventCount;
      }

      if ( real_events & WRITE ) {
        fd_ctx->triggerEvent( WRITE );
        --m_pendingEventCount;
      }
    }

    Fiber::SPtr cur { Fiber::GetThis() };
    auto raw_ptr { cur.get() };
    cur.reset();

    raw_ptr->swapOut();
  }
}

}