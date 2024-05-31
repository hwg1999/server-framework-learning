#include "sylar/address.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/tcp_server.h"
#include <unistd.h>
#include <vector>

sylar::Logger::SPtr g_logger = SYLAR_LOG_ROOT();

void run()
{
  sylar::Address::SPtr addr = sylar::Address::LoopUpAny( "127.0.0.1:8000" );
  std::vector<sylar::Address::SPtr> addrs;
  addrs.push_back( addr );

  sylar::TcpServer::SPtr tcp_server( new sylar::TcpServer );
  std::vector<sylar::Address::SPtr> fails;
  while ( !tcp_server->bind( addrs, fails ) ) {
    sleep( 2 );
  }
  tcp_server->start();
}

int main()
{
  sylar::IOManager iom { 2 };
  iom.schedule( run );
  return 0;
}