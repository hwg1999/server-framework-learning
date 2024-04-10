#include "sylar/sylar.h"
#include <unistd.h>

sylar::Logger::SPtr g_logger { SYLAR_LOG_ROOT() };

void test_fiber()
{
  static int s_count { 5 };
  SYLAR_LOG_INFO( g_logger ) << "test in fiber s_count = " << s_count;

  sleep( 1 );
  if ( --s_count >= 0 ) {
    sylar::Scheduler::GetThis()->schedule( &test_fiber, sylar::GetThreadId() );
  }
}

int main()
{
  SYLAR_LOG_INFO( g_logger ) << "main";
  sylar::Scheduler sc { 3, false, "test" };

  sc.start();
  sleep( 2 );
  SYLAR_LOG_INFO( g_logger ) << "schedule";
  sc.schedule( &test_fiber );
  sc.stop();
  SYLAR_LOG_INFO( g_logger ) << "over";

  return 0;
}
