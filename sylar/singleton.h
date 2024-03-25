#pragma once

#include <memory>
namespace sylar {
template<typename T, typename X = void, int N = 0>
class Singleton
{
public:
  static T& GetInstance()
  {
    static T instance;
    return instance;
  }
};

template<typename T, typename X = void, int N = 0>
class SingletonPtr
{
public:
  static std::shared_ptr<T> GetInstance()
  {
    static std::shared_ptr<T> instance( std::make_shared<T>() );
    return instance;
  }
};

} // namespace sylar