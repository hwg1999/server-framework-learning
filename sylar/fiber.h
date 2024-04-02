#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <ucontext.h>

namespace sylar {

class Fiber : public std::enable_shared_from_this<Fiber>
{
public:
  using SPtr = std::shared_ptr<Fiber>;

  enum State
  {
    INIT,
    HOLD,
    EXEC,
    TERM,
    READY,
    EXCEPT
  };

private:
  Fiber();

public:
  Fiber( std::function<void()> cb, std::size_t stacksize = 0 );
  ~Fiber();

  void reset( std::function<void()> cb );
  // void swapIn();
  // void swapOut();

  std::uint64_t getId() const { return m_id; }
  State getState() const { return m_state; }

  static void SetThis( Fiber* f );
  static Fiber::SPtr GetThis();
  static void YieldToReady();
  static void YieldToHold();
  static uint64_t TotalFibers();

  static void MainFunc();
  static std::uint64_t GetFiberId();

private:
  std::uint64_t m_id { 0 };
  std::uint32_t m_stacksize { 0 };
  State m_state { INIT };

  ucontext_t m_ctx;
  void* m_stack { nullptr };

  std::function<void()> m_cb;
};

}