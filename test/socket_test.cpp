#include "sylar/address.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/socket.h"
#include "sylar/sylar.h"

static sylar::Logger::SPtr g_logger = SYLAR_LOG_ROOT();

void test_socket()
{
  sylar::IPAddress::SPtr addr = sylar::Address::LookUpAnyIPAddress( "www.baidu.com" );
  if ( addr ) {
    SYLAR_LOG_INFO( g_logger ) << "get address: " << addr->toString();
  } else {
    SYLAR_LOG_ERROR( g_logger ) << "get address fail";
    return;
  }

  sylar::Socket::SPtr sock = sylar::Socket::CreateTCP( addr );
  addr->setPort( 80 );
  SYLAR_LOG_INFO( g_logger ) << "addr=" << addr->toString();
  if ( int ret = sock->connect( addr ); ret ) {
    SYLAR_LOG_ERROR( g_logger ) << "connect " << addr->toString() << " fail";
    return;
  } else {
    SYLAR_LOG_INFO( g_logger ) << "connect " << addr->toString() << " connected";
  }

  const char buff[] = "GET / HTTP/1.0\r\n\r\n";
  int ret = sock->send( buff, sizeof( buff ) );
  if ( ret <= 0 ) {
    SYLAR_LOG_INFO( g_logger ) << "send fail ret = " << ret;
    return;
  }

  std::string bufs;
  bufs.resize( 4096 );
  ret = sock->recv( bufs.data(), bufs.size() );

  if ( ret <= 0 ) {
    SYLAR_LOG_INFO( g_logger ) << "recv fail ret = " << ret;
    return;
  }

  bufs.resize( ret );
  SYLAR_LOG_INFO( g_logger ) << bufs;
}

int main()
{
  sylar::IOManager iom;
  iom.schedule( &test_socket );
  return 0;
}