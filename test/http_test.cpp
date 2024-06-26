#include "sylar/http/http.h"
#include "sylar/log.h"
#include <iostream>

void test_request()
{
  sylar::http::HttpRequest::SPtr req( new sylar::http::HttpRequest );
  req->setHeader( "host", "www.sylar.top" );
  req->setBody( "hello sylar" );
  req->dump( std::cout ) << std::endl;
}

void test_response()
{
  sylar::http::HttpResponse::SPtr rsp( new sylar::http::HttpResponse );
  rsp->setHeader( "X-X", "sylar" );
  rsp->setBody( "hello sylar" );
  rsp->setStatus( (sylar::http::HttpStatus)400 );
  rsp->setClose( false );

  rsp->dump( std::cout ) << std::endl;
}

int main()
{
  test_request();
  test_response();

  return 0;
}