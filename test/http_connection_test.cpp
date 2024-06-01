#include "sylar/address.h"
#include "sylar/http/http.h"
#include "sylar/http/http_connection.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/socket.h"
#include <fstream>
#include <iostream>
#include <memory>

static sylar::Logger::SPtr g_logger = SYLAR_LOG_ROOT();

void test_pool()
{
  sylar::http::HttpConnectionPool::SPtr pool(
    new sylar::http::HttpConnectionPool( "www.sylar.top", "", 80, 10, 1000 * 30, 5 ) );
  sylar::IOManager::GetThis()->addTimer(
    1000,
    [pool]() {
      auto r = pool->doGet( "/", 300 );
      SYLAR_LOG_INFO( g_logger ) << r->toString();
    },
    true );
}

void run()
{
  sylar::Address::SPtr addr { sylar::Address::LookUpAnyIPAddress( "www.sylar.top:80" ) };
  if ( !addr ) {
    SYLAR_LOG_INFO( g_logger ) << "get addr error";
    return;
  }

  sylar::Socket::SPtr sock { sylar::Socket::CreateTCP( addr ) };
  bool ret = sock->connect( addr );
  if ( !ret ) {
    SYLAR_LOG_INFO( g_logger ) << "connect " << *addr << " failed";
    return;
  }

  sylar::http::HttpConnection::SPtr conn { std::make_shared<sylar::http::HttpConnection>( sock ) };
  sylar::http::HttpRequest::SPtr req { std::make_shared<sylar::http::HttpRequest>() };
  req->setPath( "/blog/" );
  req->setHeader( "host", "www.sylar.top" );
  SYLAR_LOG_INFO( g_logger ) << "req:" << std::endl << *req;

  conn->sendRequest( req );
  auto rsp = conn->recvResponse();

  if ( !rsp ) {
    SYLAR_LOG_INFO( g_logger ) << "recv response error";
    return;
  }

  SYLAR_LOG_INFO( g_logger ) << "rsp:" << std::endl << *rsp;

  std::ofstream ofs { "rsp.dat" };
  ofs << *rsp;

  SYLAR_LOG_INFO( g_logger ) << "========================";

  auto r = sylar::http::HttpConnection::DoGet( "http://www.sylar.top/blog/", 300 );
  SYLAR_LOG_INFO( g_logger ) << "result=" << r->result_ << " error=" << r->error_
                             << " rsp=" << ( r->response_ ? r->response_->toString() : "" );

  SYLAR_LOG_INFO( g_logger ) << "=========================";
  test_pool();
}

int main( int argc, char** argv )
{
  sylar::IOManager iom { 2 };
  iom.schedule( run );
  return 0;
}