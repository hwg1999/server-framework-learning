#pragma once

#include "sylar/http/http_servlet.h"
#include "sylar/iomanager.h"
#include "sylar/socket.h"
#include "sylar/tcp_server.h"

namespace sylar {
namespace http {

class HttpServer : public TcpServer
{
public:
  using SPtr = std::shared_ptr<HttpServer>;

  HttpServer( bool keepAlive = false,
              sylar::IOManager* worker = sylar::IOManager::GetThis(),
              sylar::IOManager* ioWorker = sylar::IOManager::GetThis(),
              sylar::IOManager* accept_worker = sylar::IOManager::GetThis() );

  ServletDispatch::SPtr getServletDisaptch() const { return dispatch_; }

  void setServletDispatch( ServletDispatch::SPtr dispatch ) { dispatch_ = dispatch; }

  virtual void setName( const std::string& val ) override;

protected:
  virtual void handleClient( Socket::SPtr client ) override;

private:
  bool isKeepAlive_;
  ServletDispatch::SPtr dispatch_;
};

}
}