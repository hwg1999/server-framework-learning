#include "sylar/address.h"
#include "sylar/log.h"
#include <cstddef>
#include <vector>

sylar::Logger::SPtr g_logger = SYLAR_LOG_ROOT();

void test()
{
  std::vector<sylar::Address::SPtr> addrs;

  SYLAR_LOG_INFO( g_logger ) << "begin";
  bool val = sylar::Address::LookUp( addrs, "www.sylar.top" );
  SYLAR_LOG_INFO( g_logger ) << "end";

  if ( !val ) {
    SYLAR_LOG_ERROR( g_logger ) << "lookup fail";
    return;
  }

  for ( std::size_t i = 0; i < addrs.size(); ++i ) {
    SYLAR_LOG_INFO( g_logger ) << i << " - " << addrs[i]->toString();
  }
}

void test_iface()
{
  std::multimap<std::string, std::pair<sylar::Address::SPtr, uint32_t>> results;

  bool v = sylar::Address::GetInterfaceAddress( results );
  if ( !v ) {
    SYLAR_LOG_ERROR( g_logger ) << "GetInterfaceAddresses fail";
    return;
  }

  for ( auto& i : results ) {
    SYLAR_LOG_INFO( g_logger ) << i.first << " - " << i.second.first->toString() << " - " << i.second.second;
  }
}

void test_ipv4()
{
  auto addr = sylar::IPAddress::Create( "127.0.0.1" );
  if ( addr ) {
    SYLAR_LOG_INFO( g_logger ) << addr->toString();
  }
}

int main()
{
  test();
  test_iface();
  test_ipv4();

  return 0;
}