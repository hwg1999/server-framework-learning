#pragma once

namespace sylar {
class Noncopyable
{
public:
  Noncopyable() = default;
  ~Noncopyable() = default;
  Noncopyable& operator=( const Noncopyable& ) = delete;
  Noncopyable& operator=( Noncopyable&& ) = delete;
};
}