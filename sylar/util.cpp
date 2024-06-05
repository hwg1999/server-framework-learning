#include "util.h"
#include "sylar/fiber.h"
#include "sylar/log.h"
#include <ctime>
#include <execinfo.h>
#include <sstream>
#include <sys/syscall.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

namespace sylar {

sylar::Logger::SPtr g_logger = SYLAR_LOG_NAME( "system" );

pid_t GetThreadId()
{
  return syscall( SYS_gettid );
}
std::uint32_t GetFiberId()
{
  return sylar::Fiber::GetFiberId();
}

void Backtrace( std::vector<std::string>& bt, int size, int skip )
{
  void** array = (void**)malloc( ( sizeof( void* ) * size ) );
  std::size_t btSize = ::backtrace( array, size );

  char** strings = backtrace_symbols( array, btSize );
  if ( nullptr == strings ) {
    SYLAR_LOG_ERROR( g_logger ) << "backtrace_symbols error";
    return;
  }

  for ( std::size_t i = skip; i < btSize; ++i ) {
    bt.push_back( strings[i] );
  }

  std::free( strings );
  std::free( array );
}

std::string BacktraceToString( int size, int skip, const std::string& prefix )
{
  std::vector<std::string> bt;
  Backtrace( bt, size, skip );
  std::stringstream ss;
  for ( std::size_t i = 0; i < bt.size(); ++i ) {
    ss << prefix << bt[i] << std::endl;
  }
  return ss.str();
}

std::uint64_t GetCurrentMS()
{
  struct timeval tv;
  gettimeofday( &tv, nullptr );
  return tv.tv_sec * 1000ul + tv.tv_usec / 1000;
}

std::uint64_t GetCurrentUS()
{
  struct timeval tv;
  gettimeofday( &tv, nullptr );
  return tv.tv_sec * 1000 * 1000ul + tv.tv_usec;
}

uint64_t GetElapsedMS()
{
  struct timespec ts = { 0 };
  clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
  return ts.tv_sec * 1000 + ts.tv_nsec / 1000'000;
}

} // namespace sylar