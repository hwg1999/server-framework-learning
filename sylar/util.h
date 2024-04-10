#pragma once
#include <cstdint>
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
} // namespace sylar