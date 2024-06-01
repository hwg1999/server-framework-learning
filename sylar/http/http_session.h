#pragma once

#include "sylar/http/http.h"
#include "sylar/socket.h"
#include "sylar/socket_stream.h"
#include <memory>

namespace sylar {
namespace http {

class HttpSession : public SocketStream
{
public:
  using SPtr = std::shared_ptr<HttpSession>;

  HttpSession( Socket::SPtr sock, bool owner = true );

  HttpRequest::SPtr recvRequest();

  int sendResponse( HttpResponse::SPtr rsp );
};

}
}