#include "sylar/http/http.h"
#include "sylar/http/http_parser.h"
#include "sylar/log.h"

static sylar::Logger::SPtr g_logger = SYLAR_LOG_ROOT();

const char* test_request_data = "POST / HTTP/1.1\r\n"
                                "Host: www.sylar.top\r\n"
                                "Content-Length: 10\r\n\r\n"
                                "1234567890";

void test_request()
{
  sylar::http::HttpRequestParser parser;
  std::string tmp = test_request_data;
  SYLAR_LOG_INFO( g_logger ) << "<test_request>:" << tmp;
  size_t s = parser.execute( tmp.data(), tmp.size() );
  if ( parser.hasError() ) {
    SYLAR_LOG_ERROR( g_logger ) << "parser execute fail";
  } else {
    sylar::http::HttpRequest::SPtr req = parser.getData();
    SYLAR_LOG_INFO( g_logger ) << req->toString();
  }
}

const char* test_response_data = "HTTP/1.1 200 OK\r\n"
                                 "Date: Tue, 04 Jun 2019 15:43:56 GMT\r\n"
                                 "Server: Apache\r\n"
                                 "Last-Modified: Tue, 12 Jan 2010 13:48:00 GMT\r\n"
                                 "ETag: \"51-47cf7e6ee8400\"\r\n"
                                 "Accept-Ranges: bytes\r\n"
                                 "Content-Length: 81\r\n"
                                 "Cache-Control: max-age=86400\r\n"
                                 "Expires: Wed, 05 Jun 2019 15:43:56 GMT\r\n"
                                 "Connection: Close\r\n"
                                 "Content-Type: text/html\r\n\r\n"
                                 "<html>\r\n"
                                 "<meta http-equiv=\"refresh\" content=\"0;url=http://www.baidu.com/\">\r\n"
                                 "</html>\r\n";

void test_response()
{
  sylar::http::HttpResponseParser parser;
  std::string tmp = test_response_data;
  SYLAR_LOG_INFO( g_logger ) << "<test_response>:" << tmp;
  parser.execute( tmp.data(), tmp.size() );
  if ( parser.hasError() ) {
    SYLAR_LOG_ERROR( g_logger ) << "parser execute fail";
  } else {
    sylar::http::HttpResponse::SPtr rsp = parser.getData();
    SYLAR_LOG_INFO( g_logger ) << rsp->toString();
  }
}

int main()
{
  test_request();
  SYLAR_LOG_INFO( g_logger ) << "---------";
  test_response();
}
