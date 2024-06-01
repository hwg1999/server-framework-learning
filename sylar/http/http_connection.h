#pragma once

#include "sylar/http/http.h"
#include "sylar/socket.h"
#include "sylar/socket_stream.h"
#include "sylar/thread.h"
#include "sylar/uri.h"
#include <atomic>
#include <list>
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

  static HttpResult::SPtr DoGet( Uri::SPtr uri,
                                 uint64_t timeoutMs,
                                 const std::map<std::string, std::string>& headers = {},
                                 const std::string& body = "" );

  static HttpResult::SPtr DoPost( const std::string& url,
                                  uint64_t timeout_ms,
                                  const std::map<std::string, std::string>& headers = {},
                                  const std::string& body = "" );

  static HttpResult::SPtr DoPost( Uri::SPtr uri,
                                  uint64_t timeoutMs,
                                  const std::map<std::string, std::string>& headers = {},
                                  const std::string& body = "" );

  static HttpResult::SPtr DoRequest( HttpMethod method,
                                     const std::string& url,
                                     uint64_t timeoutMs,
                                     const std::map<std::string, std::string>& headers = {},
                                     const std::string& body = "" );

  static HttpResult::SPtr DoRequest( HttpMethod method,
                                     Uri::SPtr uri,
                                     uint64_t timeoutMs,
                                     const std::map<std::string, std::string>& headers = {},
                                     const std::string& body = "" );

  static HttpResult::SPtr DoRequest( HttpRequest::SPtr req, Uri::SPtr uri, uint64_t timeoutMs );

  HttpConnection( Socket::SPtr sock, bool owner = true );

  ~HttpConnection();

  HttpResponse::SPtr recvResponse();

  int sendRequest( HttpRequest::SPtr req );

private:
  uint64_t createTime_ { 0 };
  uint64_t request_ { 0 };
};

class HttpConnectionPool
{
public:
  using SPtr = std::shared_ptr<HttpConnectionPool>;
  using MutexType = Mutex;

  HttpConnectionPool( const std::string& host,
                      const std::string& vhost,
                      uint32_t port,
                      uint32_t max_size,
                      uint32_t max_alive_time,
                      uint32_t max_request );

  HttpConnection::SPtr getConnection();

  HttpResult::SPtr doGet( const std::string& url,
                          uint64_t timeout_ms,
                          const std::map<std::string, std::string>& headers = {},
                          const std::string& body = "" );

  HttpResult::SPtr doGet( Uri::SPtr uri,
                          uint64_t timeout_ms,
                          const std::map<std::string, std::string>& headers = {},
                          const std::string& body = "" );

  HttpResult::SPtr doPost( const std::string& url,
                           uint64_t timeout_ms,
                           const std::map<std::string, std::string>& headers = {},
                           const std::string& body = "" );

  HttpResult::SPtr doPost( Uri::SPtr uri,
                           uint64_t timeout_ms,
                           const std::map<std::string, std::string>& headers = {},
                           const std::string& body = "" );

  HttpResult::SPtr doRequest( HttpMethod method,
                              const std::string& url,
                              uint64_t timeout_ms,
                              const std::map<std::string, std::string>& headers = {},
                              const std::string& body = "" );

  HttpResult::SPtr doRequest( HttpMethod method,
                              Uri::SPtr uri,
                              uint64_t timeout_ms,
                              const std::map<std::string, std::string>& headers = {},
                              const std::string& body = "" );

  HttpResult::SPtr doRequest( HttpRequest::SPtr req, uint64_t timeout_ms );

private:
  static void ReleasePtr( HttpConnection* ptr, HttpConnectionPool* pool );

private:
  std::string host_;
  std::string vhost_;
  uint32_t port_;
  uint32_t maxSize_;
  uint32_t maxAliveTime_;
  uint32_t maxRequest_;
  MutexType mutex_;
  std::list<HttpConnection*> conns_;
  std::atomic<int32_t> total_ { 0 };
};

}

}