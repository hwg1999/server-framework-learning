#include "util.h"
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

namespace sylar {
pid_t GetThreadId()
{
  return syscall( SYS_gettid );
}
std::uint32_t GetFiberId()
{
  return 0;
}
} // namespace sylar