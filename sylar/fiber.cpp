#include "fiber.h"
#include "sylar/config.h"
#include "sylar/log.h"
#include "sylar/macro.h"
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <functional>
#include <memory>
#include <ucontext.h>

namespace sylar {

static Logger::SPtr g_logger = SYLAR_LOG_NAME( "system" );

static std::atomic<std::uint64_t> s_fiber_id { 0 };
static std::atomic<std::uint64_t> s_fiber_count { 0 };

static thread_local Fiber* t_fiber { nullptr };
static thread_local Fiber::SPtr t_threadFiber { nullptr };

static ConfigVar<std::uint32_t>::SPtr g_fiber_stack_size
  = Config::Lookup<std::uint32_t>( "fiber.stack.size", 1024 * 1024, "fiber stack size" );

class MallocStackAllocator
{
public:
  static void* Alloc( std::size_t size ) { return std::malloc( size ); }

  static void Dealloc( void* ptr, std::size_t size ) { std::free( ptr ); }
};

using StackAllocator = MallocStackAllocator;

std::uint64_t Fiber::GetFiberId()
{
  if ( t_fiber ) {
    return t_fiber->GetFiberId();
  }
  return 0;
}

Fiber::Fiber()
{
  m_state = EXEC;
  SetThis( this );

  if ( getcontext( &m_ctx ) ) {
    SYLAR_ASSERT2( false, "getcontext" );
  }

  ++s_fiber_count;

  SYLAR_LOG_DEBUG( g_logger ) << "Fiber::Fiber";
}

Fiber::Fiber( std::function<void()> cb, std::size_t stacksize ) : m_id( ++s_fiber_id ), m_cb( cb )
{
  ++s_fiber_count;
  m_stacksize = stacksize ? stacksize : g_fiber_stack_size->getValue();

  m_stack = StackAllocator::Alloc( m_stacksize );
  if ( getcontext( &m_ctx ) ) {
    SYLAR_ASSERT2( false, "getcontext" );
  }
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext( &m_ctx, &Fiber::MainFunc, 0 );
}

Fiber::~Fiber()
{
  --s_fiber_count;
  if ( m_stack ) {
    SYLAR_ASSERT( TERM == m_state || INIT == m_state || EXCEPT == m_state );
    StackAllocator::Dealloc( m_stack, m_stacksize );
  } else {
    SYLAR_ASSERT( !m_cb );
    SYLAR_ASSERT( EXEC == m_state );

    Fiber* cur = t_fiber;
    if ( this == cur ) {
      SetThis( nullptr );
    }
  }

  SYLAR_LOG_DEBUG( g_logger ) << "Fiber::~Fiber id = " << m_id;
}

// 重置协程函数，并重置状态
// INIT，TERM
void Fiber::reset( std::function<void()> cb )
{
  SYLAR_ASSERT( m_stack );
  SYLAR_ASSERT( TERM == m_state || EXCEPT == m_state || INIT == m_state );
  m_cb = cb;
  if ( getcontext( &m_ctx ) ) {
    SYLAR_ASSERT2( false, "getcontext" );
  }
  m_ctx.uc_link = nullptr;
  m_ctx.uc_stack.ss_sp = m_stack;
  m_ctx.uc_stack.ss_size = m_stacksize;

  makecontext( &m_ctx, Fiber::MainFunc, 0 );
  m_state = INIT;
}

// // 切换到当前协程执行
// void Fiber::swapIn()
// {
//   SetThis( this );
//   SYLAR_ASSERT( m_state != EXEC );
//   m_state = EXEC;
//   if(swapcontext(&, const ucontext_t *__restrict ucp))
// }

// // 切换到后台执行
// void swapOut();

// 设置当前协程
void Fiber::SetThis( Fiber* f )
{
  t_fiber = f;
}

// 返回当前协程
Fiber::SPtr Fiber::GetThis()
{
  if ( t_fiber ) {
    return t_fiber->shared_from_this();
  }

  Fiber::SPtr main_fiber { new Fiber };
  SYLAR_ASSERT( t_fiber == main_fiber.get() );
  t_threadFiber = main_fiber;
  return t_fiber->shared_from_this();
}

// // 协程切换到后台，并且设置为Ready状态
// static void YieldToReady();
// // 协程切换到后台，并且设置为Hold状态
// static void YieldToHold();

// 总协程数
uint64_t Fiber::TotalFibers()
{
  return s_fiber_count;
}

void Fiber::MainFunc() {}

}