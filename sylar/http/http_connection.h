#pragma once

#include "sylar/http/http.h"
#include "sylar/socket_stream.h"
#include <map>
#include <memory>
#include <string>

namespace sylar {
namespace http {

struct HttpResult
{
  using SPtr = std::shared_ptr<HttpResult>;

  enum class Error
  {
    OK = 0,
    INVALID_URL,
    INVALID_HOST,
    CONNECT_FAIL,
    SEND_CLOSE_BY_PEER,
    SEND_SOCKET_ERROR,
    TIMEOUT,
    CREATE_SOCKET_ERROR,
    POOL_GET_CONNECTION,
    POOL_INVALID_CONNECTION,
  };

  HttpResult( int result, HttpResponse::SPtr response, const std::string& error )
    : result_ { result }, response_ { response }, error_ { error }
  {}

  std::string toString() const;

  int result_;
  HttpResponse::SPtr response_;
  std::string error_;
};

class HttpConnectionPool;

class HttpConnection : public SocketStream
{
  friend class HttpConnectionPool;

public:
  using SPtr = std::shared_ptr<HttpConnection>;

  static HttpResult::SPtr DoGet( const std::string& url,
                                 uint64_t timeout_ms,
                                 const std::map<std::string, std::string>& headers = {},
                                 const std::string& body = "" );

    

private:
  uint64_t createTime_ { 0 };
  uint64_t request_ { 0 };
};

}

}