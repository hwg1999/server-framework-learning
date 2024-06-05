#include "sylar/http/http_connection.h"
#include "sylar/address.h"
#include "sylar/http/http.h"
#include "sylar/http/http_parser.h"
#include "sylar/log.h"
#include "sylar/socket.h"
#include "sylar/socket_stream.h"
#include "sylar/uri.h"
#include "sylar/util.h"
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <strings.h>
#include <vector>

namespace sylar {
namespace http {

static sylar::Logger::SPtr g_logger = SYLAR_LOG_NAME( "system" );

std::string HttpResult::toString() const
{
  std::stringstream ss;
  ss << "[HttpResult result=" << result_ << " error=" << error_
     << " response=" << ( response_ ? response_->toString() : "nullptr" ) << "]";
  return ss.str();
}

HttpConnection::HttpConnection( Socket::SPtr sock, bool owner ) : SocketStream { sock, owner } {}

HttpConnection::~HttpConnection()
{
  SYLAR_LOG_DEBUG( g_logger ) << "HttpConnection::~HttpConnection";
}

HttpResponse::SPtr HttpConnection::recvResponse()
{
  HttpResponseParser::SPtr parser { std::make_shared<HttpResponseParser>() };
  uint64_t buf_size = HttpResponseParser::GetHttpResponseBufferSize();
  std::vector<char> buffer( buf_size );
  int offset = 0;
  do {
    int len = read( buffer.data() + offset, buf_size - offset );
    if ( len <= 0 ) {
      close();
      return nullptr;
    }

    len += offset;
    buffer[len] = '\0';

    size_t nparse = parser->execute( buffer.data(), len );
    if ( parser->hasError() ) {
      close();
      return nullptr;
    }

    offset = len - nparse;
    if ( offset == (int)buf_size ) {
      close();
      return nullptr;
    }

    if ( parser->isFinished() ) {
      break;
    }

  } while ( true );

  return parser->getData();
}

int HttpConnection::sendRequest( HttpRequest::SPtr rsp )
{
  std::stringstream ss;
  ss << *rsp;
  std::string data = ss.str();
  return writeFixSize( data.c_str(), data.size() );
}

HttpResult::SPtr HttpConnection::DoGet( const std::string& url,
                                        uint64_t timeout_ms,
                                        const std::map<std::string, std::string>& headers,
                                        const std::string& body )
{
  Uri::SPtr uri = Uri::Create( url );
  if ( !uri ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url );
  }

  return DoGet( uri, timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnection::DoGet( Uri::SPtr uri,
                                        uint64_t timeout_ms,
                                        const std::map<std::string, std::string>& headers,
                                        const std::string& body )
{
  return DoRequest( HttpMethod::GET, uri, timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnection::DoPost( const std::string& url,
                                         uint64_t timeout_ms,
                                         const std::map<std::string, std::string>& headers,
                                         const std::string& body )
{
  Uri::SPtr uri = Uri::Create( url );
  if ( !uri ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url );
  }
  return DoPost( uri, timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnection::DoPost( Uri::SPtr uri,
                                         uint64_t timeout_ms,
                                         const std::map<std::string, std::string>& headers,
                                         const std::string& body )
{
  return DoRequest( HttpMethod::POST, uri, timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnection::DoRequest( HttpMethod method,
                                            const std::string& url,
                                            uint64_t timeout_ms,
                                            const std::map<std::string, std::string>& headers,
                                            const std::string& body )
{
  Uri::SPtr uri = Uri::Create( url );
  if ( !uri ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::INVALID_URL, nullptr, "invalid url: " + url );
  }
  return DoRequest( method, uri, timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnection::DoRequest( HttpMethod method,
                                            Uri::SPtr uri,
                                            uint64_t timeout_ms,
                                            const std::map<std::string, std::string>& headers,
                                            const std::string& body )
{
  HttpRequest::SPtr req { std::make_shared<HttpRequest>() };
  req->setPath( uri->getPath() );
  req->setQuery( uri->getQuery() );
  req->setFragment( uri->getFragment() );
  req->setMethod( method );

  bool has_host { false };
  for ( auto& i : headers ) {
    if ( strcasecmp( i.first.c_str(), "connection" ) == 0 ) {
      if ( strcasecmp( i.second.c_str(), "keep-alive" ) == 0 ) {
        req->setClose( false );
      }
      continue;
    }

    if ( !has_host && strcasecmp( i.first.c_str(), "host" ) == 0 ) {
      has_host = !i.second.empty();
    }

    req->setHeader( i.first, i.second );
  }

  if ( !has_host ) {
    req->setHeader( "Host", uri->getHost() );
  }

  req->setBody( body );
  return DoRequest( req, uri, timeout_ms );
}

HttpResult::SPtr HttpConnection::DoRequest( HttpRequest::SPtr req, Uri::SPtr uri, uint64_t timeout_ms )
{
  Address::SPtr addr { uri->createAddress() };
  if ( !addr ) {
    return std::make_shared<HttpResult>(
      (int)HttpResult::Error::INVALID_HOST, nullptr, "invalid host: " + uri->getHost() );
  }

  Socket::SPtr sock { Socket::CreateTCP( addr ) };
  if ( !sock ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::CREATE_SOCKET_ERROR,
                                         nullptr,
                                         "create socket fail: " + addr->toString()
                                           + " errno=" + std::to_string( errno )
                                           + " errstr=" + std::string( strerror( errno ) ) );
  }

  if ( !sock->connect( addr ) ) {
    return std::make_shared<HttpResult>(
      (int)HttpResult::Error::CONNECT_FAIL, nullptr, "connect fail: " + addr->toString() );
  }
  sock->setRecvTimeout( timeout_ms );

  HttpConnection::SPtr conn { std::make_shared<HttpConnection>( sock ) };
  int ret = conn->sendRequest( req );
  if ( ret == 0 ) {
    return std::make_shared<HttpResult>(
      (int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr, "send request closed by peer: " + addr->toString() );
  }
  if ( ret < 0 ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::SEND_SOCKET_ERROR,
                                         nullptr,
                                         "send request socket error errno=" + std::to_string( errno )
                                           + " errstr=" + std::string( strerror( errno ) ) );
  }

  auto rsp = conn->recvResponse();
  if ( !rsp ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::TIMEOUT,
                                         nullptr,
                                         "recv response timeout: " + addr->toString()
                                           + " timeout_ms:" + std::to_string( timeout_ms ) );
  }

  return std::make_shared<HttpResult>( (int)HttpResult::Error::OK, rsp, "ok" );
}

HttpConnectionPool::HttpConnectionPool( const std::string& host,
                                        const std::string& vhost,
                                        uint32_t port,
                                        uint32_t max_size,
                                        uint32_t max_alive_time,
                                        uint32_t max_request )
  : host_ { host }
  , vhost_ { vhost }
  , port_ { port }
  , maxSize_ { max_size }
  , maxAliveTime_ { max_alive_time }
  , maxRequest_ { max_request }
{}

HttpConnection::SPtr HttpConnectionPool::getConnection()
{
  uint64_t now_ms { sylar::GetCurrentMS() };
  std::vector<HttpConnection*> invald_conns;
  HttpConnection* ptr { nullptr };
  MutexType::Lock lock { mutex_ };
  while ( !conns_.empty() ) {
    auto conn = *conns_.begin();
    conns_.pop_front();
    if ( !conn->isConnected() ) {
      invald_conns.push_back( conn );
      continue;
    }
    if ( ( conn->createTime_ + maxAliveTime_ ) > now_ms ) {
      invald_conns.push_back( conn );
      continue;
    }
    ptr = conn;
    break;
  }
  lock.unlock();
  for ( auto i : invald_conns ) {
    delete i;
  }

  total_ -= invald_conns.size();

  if ( !ptr ) {
    IPAddress::SPtr addr = Address::LookUpAnyIPAddress( host_ );
    if ( !addr ) {
      SYLAR_LOG_ERROR( g_logger ) << "get addr fail: " << host_;
      return nullptr;
    }
    addr->setPort( port_ );
    Socket::SPtr sock { Socket::CreateTCP( addr ) };
    if ( !sock ) {
      // SYLAR_LOG_ERROR(g_logger) < "create sock fail: " << *addr;
      return nullptr;
    }
    if ( !sock->connect( addr ) ) {
      // SYLAR_LOG_ERROR(g_logger) << "sock connect fail: " << *addr;
      return nullptr;
    }
    ptr = new HttpConnection( sock );
    ++total_;
  }

  return HttpConnection::SPtr( ptr, std::bind( HttpConnectionPool::ReleasePtr, std::placeholders::_1, this ) );
}

void HttpConnectionPool::ReleasePtr( HttpConnection* ptr, HttpConnectionPool* pool )
{
  ++ptr->request_;
  if ( !ptr->isConnected() || ( ptr->createTime_ + pool->maxAliveTime_ ) >= sylar::GetCurrentMS()
       || ( ptr->request_ >= pool->maxRequest_ ) ) {
    delete ptr;
    --pool->total_;
    return;
  }

  MutexType::Lock lock { pool->mutex_ };
  pool->conns_.push_back( ptr );
}

HttpResult::SPtr HttpConnectionPool::doGet( const std::string& url,
                                            uint64_t timeout_ms,
                                            const std::map<std::string, std::string>& headers,
                                            const std::string& body )
{
  return doRequest( HttpMethod::GET, url, timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnectionPool::doGet( Uri::SPtr uri,
                                            uint64_t timeout_ms,
                                            const std::map<std::string, std::string>& headers,
                                            const std::string& body )
{
  std::stringstream ss;
  ss << uri->getPath() << ( uri->getQuery().empty() ? "" : "?" ) << uri->getQuery()
     << ( uri->getFragment().empty() ? "" : "#" ) << uri->getFragment();
  return doGet( ss.str(), timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnectionPool::doPost( const std::string& url,
                                             uint64_t timeout_ms,
                                             const std::map<std::string, std::string>& headers,
                                             const std::string& body )
{
  return doRequest( HttpMethod::POST, url, timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnectionPool::doPost( Uri::SPtr uri,
                                             uint64_t timeout_ms,
                                             const std::map<std::string, std::string>& headers,
                                             const std::string& body )
{
  std::stringstream ss;
  ss << uri->getPath() << ( uri->getQuery().empty() ? "" : "?" ) << uri->getQuery()
     << ( uri->getFragment().empty() ? "" : "#" ) << uri->getFragment();
  return doPost( ss.str(), timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnectionPool::doRequest( HttpMethod method,
                                                const std::string& url,
                                                uint64_t timeout_ms,
                                                const std::map<std::string, std::string>& headers,
                                                const std::string& body )
{
  HttpRequest::SPtr req { std::make_shared<HttpRequest>() };
  req->setPath( url );
  req->setMethod( method );
  bool has_host { false };
  for ( auto& i : headers ) {
    if ( strcasecmp( i.first.c_str(), "connection" ) == 0 ) {
      if ( strcasecmp( i.second.c_str(), "keep-alive" ) == 0 ) {
        req->setClose( false );
      }
      continue;
    }

    if ( !has_host && strcasecmp( i.first.c_str(), "host" ) == 0 ) {
      has_host = !i.second.empty();
    }

    req->setHeader( i.first, i.second );
  }

  if ( !has_host ) {
    if ( vhost_.empty() ) {
      req->setHeader( "Host", host_ );
    } else {
      req->setHeader( "Host", vhost_ );
    }
  }

  req->setBody( body );
  return doRequest( req, timeout_ms );
}

HttpResult::SPtr HttpConnectionPool::doRequest( HttpMethod method,
                                                Uri::SPtr uri,
                                                uint64_t timeout_ms,
                                                const std::map<std::string, std::string>& headers,
                                                const std::string& body )
{
  std::stringstream ss;
  ss << uri->getPath() << ( uri->getQuery().empty() ? "" : "?" ) << uri->getQuery()
     << ( uri->getFragment().empty() ? "" : "#" ) << uri->getFragment();
  return doRequest( method, ss.str(), timeout_ms, headers, body );
}

HttpResult::SPtr HttpConnectionPool::doRequest( HttpRequest::SPtr req, uint64_t timeout_ms )
{
  auto conn = getConnection();
  if ( !conn ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::POOL_GET_CONNECTION,
                                         nullptr,
                                         "pool host:" + host_ + " port:" + std::to_string( port_ ) );
  }
  auto sock = conn->getSocket();
  if ( !sock ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::POOL_INVALID_CONNECTION,
                                         nullptr,
                                         "pool host:" + host_ + " port:" + std::to_string( port_ ) );
  }
  sock->setRecvTimeout( timeout_ms );
  int rt = conn->sendRequest( req );
  if ( rt == 0 ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::SEND_CLOSE_BY_PEER,
                                         nullptr,
                                         "send request closed by peer: " + sock->getRemoteAddress()->toString() );
  }
  if ( rt < 0 ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::SEND_SOCKET_ERROR,
                                         nullptr,
                                         "send request socket error errno=" + std::to_string( errno )
                                           + " errstr=" + std::string( strerror( errno ) ) );
  }
  auto rsp = conn->recvResponse();
  if ( !rsp ) {
    return std::make_shared<HttpResult>( (int)HttpResult::Error::TIMEOUT,
                                         nullptr,
                                         "recv response timeout: " + sock->getRemoteAddress()->toString()
                                           + " timeout_ms:" + std::to_string( timeout_ms ) );
  }
  return std::make_shared<HttpResult>( (int)HttpResult::Error::OK, rsp, "ok" );
}

}
}