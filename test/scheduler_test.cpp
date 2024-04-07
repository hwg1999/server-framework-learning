#include "sylar/sylar.h"

sylar::Logger::SPtr g_logger { SYLAR_LOG_ROOT() };

int main()
{
  sylar::Scheduler sc;
  sc.start();
  sc.stop();
  return 0;
}
