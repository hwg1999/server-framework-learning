#include "sylar/uri.h"
#include <iostream>

int main()
{
  sylar::Uri::SPtr uri = sylar::Uri::Create( "http://admin@www.sylar.top" );
  std::cout << uri->toString() << std::endl;
  auto addr = uri->createAddress();
  std::cout << *addr << std::endl;
  return 0;
}