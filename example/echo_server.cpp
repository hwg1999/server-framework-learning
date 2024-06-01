#include "sylar/address.h"
#include "sylar/bytearray.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/socket.h"
#include "sylar/tcp_server.h"
#include <bits/types/struct_iovec.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>

static sylar::Logger::SPtr g_logger = SYLAR_LOG_ROOT();

class EchoServer : public sylar::TcpServer
{
public:
  EchoServer( int type );
  void handleClient( sylar::Socket::SPtr clinet );

private:
  int m_type { 0 };
};

EchoServer::EchoServer( int type ) : m_type { type } {}

void EchoServer::handleClient( sylar::Socket::SPtr client )
{
  SYLAR_LOG_INFO( g_logger ) << "handleCLient" << *client;
  sylar::ByteArray::SPtr ba( new sylar::ByteArray );
  while ( true ) {
    ba->clear();
    std::vector<iovec> iovs;
    ba->getWriteBuffers( iovs, 1024 );

    int ret = client->recv( iovs.data(), iovs.size() );
    if ( ret == 0 ) {
      SYLAR_LOG_INFO( g_logger ) << "client close: " << *client;
      break;
    } else if ( ret < 0 ) {
      SYLAR_LOG_INFO( g_logger ) << "client error ret=" << ret << " errno=" << errno
                                 << " errstr=" << strerror( errno );
      break;
    }

    ba->setPosition( ba->getPosition() + ret );
    ba->setPosition( 0 );

    if ( m_type == 1 ) {
      std::cout << ba->toString() << std::endl;
    } else {
      std::cout << ba->toHexString() << std::endl;
    }

    std::cout.flush();
  }
}

int type = 1;

void run()
{
  SYLAR_LOG_INFO( g_logger ) << "server type=" << type;
  EchoServer::SPtr es( new EchoServer( type ) );
  auto addr = sylar::Address::LoopUpAny( "127.0.0.1:8000" );
  while ( !es->bind( addr ) ) {
    sleep( 2 );
  }
  es->start();
}

int main( int argc, char** argv )
{
  if ( argc < 2 ) {
    SYLAR_LOG_INFO( g_logger ) << "used as[" << argv[0] << " -t] or [" << argv[0] << " -b]";
    return 0;
  }

  if ( !strcmp( argv[1], "-b" ) ) {
    type = 2;
  }

  sylar::IOManager iom( 2 );
  iom.schedule( run );
  return 0;
}