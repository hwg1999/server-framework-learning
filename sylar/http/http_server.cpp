#include "sylar/http/http_server.h"
#include "sylar/http/http.h"
#include "sylar/http/http_servlet.h"
#include "sylar/http/http_session.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include "sylar/socket.h"
#include "sylar/tcp_server.h"
#include <memory>

namespace sylar {
namespace http {

static sylar::Logger::SPtr g_logger = SYLAR_LOG_NAME( "system" );

HttpServer::HttpServer( bool keepAlive,
                        sylar::IOManager* worker,
                        sylar::IOManager* ioWorker,
                        sylar::IOManager* acceptWorker )
  : TcpServer { ioWorker, acceptWorker }
  , isKeepAlive_( keepAlive )
  , dispatch_ { std::make_shared<ServletDispatch>() }
{
  m_type = "http";
}

void HttpServer::setName( const std::string& v )
{
  TcpServer::setName( v );
  dispatch_->setDeafault( std::make_shared<NotFoundServlet>( v ) );
}

void HttpServer::handleClient( Socket::SPtr client )
{
  SYLAR_LOG_DEBUG( g_logger ) << "handleClient " << *client;
  HttpSession::SPtr session( std::make_shared<HttpSession>( client ) );

  do {
    auto req = session->recvRequest();
    if ( !req ) {
      SYLAR_LOG_DEBUG( g_logger ) << "recv http request fail, errno=" << errno << " errstr=" << strerror( errno )
                                  << "client:" << *client << "keep_alive=" << isKeepAlive_;
      break;
    }

    HttpResponse::SPtr rsp { std::make_shared<HttpResponse>( req->getVersion(), req->isClose() || !isKeepAlive_ ) };
    rsp->setHeader( "Server", getName() );
    dispatch_->handle( req, rsp, session );
    session->sendResponse( rsp );

    if ( !isKeepAlive_ || req->isClose() ) {
      break;
    }
  } while ( true );
  
  session->close();
}

}

}