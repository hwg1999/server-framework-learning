#include "sylar/address.h"
#include "sylar/http/http_server.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include <memory>
#include <unistd.h>

static sylar::Logger::SPtr g_logger = SYLAR_LOG_ROOT();

#define XX( ... ) #__VA_ARGS__

void run()
{
  g_logger->setLevel( sylar::LogLevel::INFO );
  sylar::http::HttpServer::SPtr server { std::make_shared<sylar::http::HttpServer>( true ) };
  sylar::Address::SPtr addr = sylar::Address::LookUpAnyIPAddress( "127.0.0.1:8000" );
  while ( !server->bind( addr ) ) {
    sleep( 2 );
  }

  auto sd = server->getServletDisaptch();
  sd->addServlet( "/sylar/xx",
                  []( sylar::http::HttpRequest::SPtr req,
                      sylar::http::HttpResponse::SPtr rsp,
                      sylar::http::HttpSession::SPtr session ) {
                    rsp->setBody( req->toString() );
                    return 0;
                  } );

  sd->addGlobServlet( "/sylar/*",
                      []( sylar::http::HttpRequest::SPtr req,
                          sylar::http::HttpResponse::SPtr rsp,
                          sylar::http::HttpSession::SPtr session ) {
                        rsp->setBody( "Glob:\r\n" + req->toString() );
                        return 0;
                      } );

  sd->addGlobServlet( "/sylarx/*",
                      []( sylar::http::HttpRequest::SPtr req,
                          sylar::http::HttpResponse::SPtr rsp,
                          sylar::http::HttpSession::SPtr session ) {
                        rsp->setBody( XX( <html><head> < title > 404 Not Found</ title></ head><body><center> < h1
                                            > 404 Not Found</ h1></ center><hr><center> nginx / 1.16.0 < / center
                                            > </ body></ html> < !--a padding to disable MSIE
                                          and Chrome friendly error page-- > < !--a padding to disable MSIE
                                          and Chrome friendly error page-- > < !--a padding to disable MSIE
                                          and Chrome friendly error page-- > < !--a padding to disable MSIE
                                          and Chrome friendly error page-- > < !--a padding to disable MSIE
                                          and Chrome friendly error page-- > < !--a padding to disable MSIE
                                          and Chrome friendly error page-- > ) );
                        return 0;
                      } );

  server->start();
}

int main()
{
  sylar::IOManager iom { 4 };
  iom.schedule( run );
  return 0;
}