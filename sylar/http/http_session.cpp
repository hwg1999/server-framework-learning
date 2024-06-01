#include "sylar/http/http_session.h"
#include "sylar/http/http.h"
#include "sylar/http/http_parser.h"
#include "sylar/socket.h"
#include "sylar/socket_stream.h"
#include <memory>
#include <sstream>
#include <unistd.h>
#include <vector>

namespace sylar {
namespace http {

HttpSession::HttpSession( Socket::SPtr sock, bool owner ) : SocketStream { sock, owner } {}

HttpRequest::SPtr HttpSession::recvRequest()
{
  HttpRequestParser::SPtr parser { std::make_shared<HttpRequestParser>() };
  uint64_t buf_size = HttpRequestParser::GetHttpRequestBufferSize();
  std::vector<char> buffer( buf_size );
  int offset = 0;
  do {
    int len = read( buffer.data() + offset, buf_size - offset );
    if ( len <= 0 ) {
      close();
      return nullptr;
    }
    len += offset;
    size_t nparse = parser->execute( buffer.data(), len );
    if ( parser->hasError() ) {
      close();
      return nullptr;
    }
    offset = len - nparse;
    if ( offset == buf_size ) {
      close();
      return nullptr;
    }

    if ( parser->isFinished() ) {
      break;
    }

  } while ( true );

  parser->getData()->init();
  return parser->getData();
}

int HttpSession::sendResponse( HttpResponse::SPtr rsp )
{
  std::stringstream ss;
  ss << *rsp;
  std::string data = ss.str();
  return writeFixSize( data.c_str(), data.size() );
}

}
};