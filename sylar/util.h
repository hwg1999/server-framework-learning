#pragma once
#include <cstdint>
#include <cxxabi.h>
#include <pthread.h>
#include <string>
#include <vector>

namespace sylar {

pid_t GetThreadId();
std::uint32_t GetFiberId();

void Backtrace( std::vector<std::string>& bt, int size = 64, int skip = 1 );
std::string BacktraceToString( int size = 64, int skip = 2, const std::string& prefix = "" );

std::uint64_t GetCurrentMS();
std::uint64_t GetCurrentUS();

template<typename T>
const char* TypeToName()
{
  static const char* s_name = abi::__cxa_demangle( typeid( T ).name(), nullptr, nullptr, nullptr );
  return s_name;
}

uint64_t GetElapsedMS();

} // namespace sylar