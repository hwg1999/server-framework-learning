#pragma once
#include <cstdint>
#include <pthread.h>

namespace sylar {

pid_t GetThreadId();
std::uint32_t GetFiberId();

} // namespace sylar