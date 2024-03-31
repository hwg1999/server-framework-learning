#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <pthread.h>
#include <semaphore.h>
#include <string>

namespace sylar {

class Semaphore
{
public:
  Semaphore( std::uint32_t count = 0 );
  ~Semaphore();

  void wait();
  void notify();

private:
  Semaphore( const Semaphore& ) = delete;
  Semaphore( const Semaphore&& ) = delete;
  Semaphore& operator=( const Semaphore& ) = delete;

private:
  sem_t m_semaphore;
};

template<typename T>
struct ScopedLockImpl
{
public:
  ScopedLockImpl( T& mutex ) : m_mutex( mutex )
  {
    m_mutex.lock();
    m_locked = true;
  }

  ~ScopedLockImpl() { m_mutex.unlock(); }

  void lock()
  {
    if ( !m_locked ) {
      m_mutex.lock();
      m_locked = true;
    }
  }

  void unlock()
  {
    if ( m_locked ) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  T& m_mutex;
  bool m_locked;
};

template<typename T>
struct ReadScopedLockImpl
{
public:
  ReadScopedLockImpl( T& mutex ) : m_mutex( mutex )
  {
    m_mutex.rdlock();
    m_locked = true;
  }

  ~ReadScopedLockImpl() { m_mutex.unlock(); }

  void lock()
  {
    if ( !m_locked ) {
      m_mutex.rdlock();
      m_locked = true;
    }
  }

  void unlock()
  {
    if ( m_locked ) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  T& m_mutex;
  bool m_locked;
};

template<typename T>
struct WriteScopedLockImpl
{
public:
  WriteScopedLockImpl( T& mutex ) : m_mutex( mutex )
  {
    m_mutex.wrlock();
    m_locked = true;
  }

  ~WriteScopedLockImpl() { m_mutex.unlock(); }

  void lock()
  {
    if ( !m_locked ) {
      m_mutex.wrlock();
      m_locked = true;
    }
  }

  void unlock()
  {
    if ( m_locked ) {
      m_mutex.unlock();
      m_locked = false;
    }
  }

private:
  T& m_mutex;
  bool m_locked;
};

class Mutex
{
public:
  using Lock = ScopedLockImpl<Mutex>;

  Mutex() { pthread_mutex_init( &m_mutex, nullptr ); }

  ~Mutex() { pthread_mutex_destroy( &m_mutex ); }

  void lock() { pthread_mutex_lock( &m_mutex ); }

  void unlock() { pthread_mutex_unlock( &m_mutex ); }

private:
  pthread_mutex_t m_mutex;
};

class NullMutex
{
public:
  using Lock = ScopedLockImpl<NullMutex>;
  NullMutex() = default;
  ~NullMutex() = default;
  void lock() {}
  void unlock() {}
};

class RWMutex
{
public:
  using ReadLock = ReadScopedLockImpl<RWMutex>;
  using WriteLock = WriteScopedLockImpl<RWMutex>;

  RWMutex() { pthread_rwlock_init( &m_lock, nullptr ); }

  ~RWMutex() { pthread_rwlock_destroy( &m_lock ); }

  void rdlock() { pthread_rwlock_rdlock( &m_lock ); }

  void wrlock() { pthread_rwlock_wrlock( &m_lock ); }

  void unlock() { pthread_rwlock_unlock( &m_lock ); }

private:
  pthread_rwlock_t m_lock;
};

class NullRWMutex
{
public:
  using ReadLock = ReadScopedLockImpl<NullMutex>;
  using WriteLock = WriteScopedLockImpl<NullMutex>;

  NullRWMutex() = default;
  ~NullRWMutex() = default;

  void rdlock() {}
  void wrlock() {}
  void unlock() {}
};

class SpinLock
{
public:
  using Lock = ScopedLockImpl<SpinLock>;
  SpinLock() { pthread_spin_init( &m_mutex, 0 ); }

  ~SpinLock() { pthread_spin_destroy( &m_mutex ); }

  void lock() { pthread_spin_lock( &m_mutex ); }

  void unlock() { pthread_spin_unlock( &m_mutex ); }

private:
  pthread_spinlock_t m_mutex;
};

class CASLock
{
public:
  using Lock = ScopedLockImpl<CASLock>;
  CASLock() { m_mutex.clear(); }

  ~CASLock() = default;

  void lock()
  {
    while ( std::atomic_flag_test_and_set_explicit( &m_mutex, std::memory_order_acquire ) )
      ;
  }

  void unlock() { std::atomic_flag_clear_explicit( &m_mutex, std::memory_order_release ); }

private:
  std::atomic_flag m_mutex;
};

class Thread
{
public:
  using SPtr = std::shared_ptr<Thread>;
  Thread( std::function<void()> cb, const std::string& name );
  ~Thread();

  pid_t getId() const { return m_id; }
  const std::string& getName() const { return m_name; }

  void join();

  static Thread* GetThis();
  static const std::string& GetName();
  static void SetName( const std::string& name );

private:
  Thread( const Thread& ) = delete;
  Thread( Thread&& ) = delete;
  Thread& operator=( const Thread& ) = delete;

  static void* run( void* arg );

private:
  pid_t m_id { -1 };
  pthread_t m_thread { 0 };
  std::function<void()> m_cb;
  std::string m_name;

  Semaphore m_semaphore;
};

}