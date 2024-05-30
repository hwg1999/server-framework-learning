#pragma once

#include "sylar/http/http-parser/http_parser.h"
#include "sylar/http/http.h"
#include <memory>

namespace sylar {
namespace http {

class HttpRequestParser
{
public:
  using SPtr = std::shared_ptr<HttpRequestParser>;
  HttpRequestParser();
  size_t execute( char* data, size_t len );
  int isFinished() const { return m_finished; }
  void setFinished( bool val ) { m_finished = val; }
  int hasError() const { return m_error; }
  void setError( int val ) { m_error = val; }
  HttpRequest::SPtr getData() const { return m_data; }
  const http_parser& getParser() const { return m_parser; }
  const std::string& getField() const { return m_field; }
  void setField( const std::string& val ) { m_field = val; }

public:
  static uint64_t GetHttpRequestBufferSize();
  static uint64_t GetHttpRequestMaxBodySize();

private:
  http_parser m_parser;
  HttpRequest::SPtr m_data;
  int m_error;
  bool m_finished;
  std::string m_field;
};

class HttpResponseParser
{
public:
  using SPtr = std::shared_ptr<HttpResponseParser>;
  HttpResponseParser();
  size_t execute( char* data, size_t len );
  int isFinished() const { return m_finished; }
  void setFinished( bool val ) { m_finished = val; }
  int hasError() const { return m_error; }
  void setError( int val ) { m_error = val; }
  HttpResponse::SPtr getData() const { return m_data; }
  const http_parser& getParser() const { return m_parser; }
  const std::string& getField() const { return m_field; }
  void setField( const std::string& val ) { m_field = val; }

public:
  static uint64_t GetHttpResponseBufferSize();
  static uint64_t GetHttpResponseMaxBodySize();

private:
  http_parser m_parser;
  HttpResponse::SPtr m_data;
  int m_error;
  bool m_finished;
  std::string m_field;
};

}
}