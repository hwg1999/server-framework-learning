#include "sylar/sylar.h"
#include <memory>
#include <string>
#include <vector>

sylar::Logger::SPtr g_logger { SYLAR_LOG_ROOT() };

void run_in_fiber()
{
  SYLAR_LOG_INFO( g_logger ) << "run_in_fiber begin";
  sylar::Fiber::YieldToHold();
  SYLAR_LOG_INFO( g_logger ) << "run_in_fiber_end";
  sylar::Fiber::YieldToHold();
}

void test_fiber()
{
  SYLAR_LOG_INFO( g_logger ) << "main begin -1";
  {
    sylar::Fiber::GetThis();
    sylar::Fiber::SPtr fiber( std::make_shared<sylar::Fiber>( run_in_fiber ) );
    fiber->swapIn();
    SYLAR_LOG_INFO( g_logger ) << "main after swapIn";
    fiber->swapIn();
    SYLAR_LOG_INFO( g_logger ) << "main after end";
  }
  SYLAR_LOG_INFO( g_logger ) << "main after end2";
}

int main()
{
  sylar::Thread::SetName( "main" );

  std::vector<sylar::Thread::SPtr> thrs;
  for ( int i = 0; i < 1; ++i ) {
    thrs.push_back(
      sylar::Thread::SPtr( std::make_shared<sylar::Thread>( &test_fiber, "name_" + std::to_string( i ) ) ) );
  }

  for ( const auto& thr : thrs ) {
    thr->join();
  }

  return 0;
}
