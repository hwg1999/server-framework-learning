#pragma once

#include <boost/lexical_cast.hpp>
#include <map>
#include <memory>
#include <ostream>
#include <string>

namespace sylar {

namespace http {
/* Request Methods */
#define HTTP_METHOD_MAP( XX )                                                                                      \
  XX( 0, DELETE, DELETE )                                                                                          \
  XX( 1, GET, GET )                                                                                                \
  XX( 2, HEAD, HEAD )                                                                                              \
  XX( 3, POST, POST )                                                                                              \
  XX( 4, PUT, PUT )

/* Status Codes */
#define HTTP_STATUS_MAP( XX )                                                                                      \
  XX( 100, CONTINUE, Continue )                                                                                    \
  XX( 101, SWITCHING_PROTOCOLS, Switching Protocols )                                                              \
  XX( 102, PROCESSING, Processing )                                                                                \
  XX( 200, OK, OK )                                                                                                \
  XX( 201, CREATED, Created )                                                                                      \
  XX( 202, ACCEPTED, Accepted )                                                                                    \
  XX( 203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information )                                        \
  XX( 204, NO_CONTENT, No Content )                                                                                \
  XX( 205, RESET_CONTENT, Reset Content )                                                                          \
  XX( 206, PARTIAL_CONTENT, Partial Content )                                                                      \
  XX( 207, MULTI_STATUS, Multi - Status )                                                                          \
  XX( 208, ALREADY_REPORTED, Already Reported )                                                                    \
  XX( 226, IM_USED, IM Used )                                                                                      \
  XX( 300, MULTIPLE_CHOICES, Multiple Choices )                                                                    \
  XX( 301, MOVED_PERMANENTLY, Moved Permanently )                                                                  \
  XX( 302, FOUND, Found )                                                                                          \
  XX( 303, SEE_OTHER, See Other )                                                                                  \
  XX( 304, NOT_MODIFIED, Not Modified )                                                                            \
  XX( 305, USE_PROXY, Use Proxy )                                                                                  \
  XX( 307, TEMPORARY_REDIRECT, Temporary Redirect )                                                                \
  XX( 308, PERMANENT_REDIRECT, Permanent Redirect )                                                                \
  XX( 400, BAD_REQUEST, Bad Request )                                                                              \
  XX( 401, UNAUTHORIZED, Unauthorized )                                                                            \
  XX( 402, PAYMENT_REQUIRED, Payment Required )                                                                    \
  XX( 403, FORBIDDEN, Forbidden )                                                                                  \
  XX( 404, NOT_FOUND, Not Found )                                                                                  \
  XX( 405, METHOD_NOT_ALLOWED, Method Not Allowed )                                                                \
  XX( 406, NOT_ACCEPTABLE, Not Acceptable )                                                                        \
  XX( 407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required )                                          \
  XX( 408, REQUEST_TIMEOUT, Request Timeout )                                                                      \
  XX( 409, CONFLICT, Conflict )                                                                                    \
  XX( 410, GONE, Gone )                                                                                            \
  XX( 411, LENGTH_REQUIRED, Length Required )                                                                      \
  XX( 412, PRECONDITION_FAILED, Precondition Failed )                                                              \
  XX( 413, PAYLOAD_TOO_LARGE, Payload Too Large )                                                                  \
  XX( 414, URI_TOO_LONG, URI Too Long )                                                                            \
  XX( 415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type )                                                        \
  XX( 416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable )                                                          \
  XX( 417, EXPECTATION_FAILED, Expectation Failed )                                                                \
  XX( 421, MISDIRECTED_REQUEST, Misdirected Request )                                                              \
  XX( 422, UNPROCESSABLE_ENTITY, Unprocessable Entity )                                                            \
  XX( 423, LOCKED, Locked )                                                                                        \
  XX( 424, FAILED_DEPENDENCY, Failed Dependency )                                                                  \
  XX( 426, UPGRADE_REQUIRED, Upgrade Required )                                                                    \
  XX( 428, PRECONDITION_REQUIRED, Precondition Required )                                                          \
  XX( 429, TOO_MANY_REQUESTS, Too Many Requests )                                                                  \
  XX( 431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large )                                      \
  XX( 451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons )                                          \
  XX( 500, INTERNAL_SERVER_ERROR, Internal Server Error )                                                          \
  XX( 501, NOT_IMPLEMENTED, Not Implemented )                                                                      \
  XX( 502, BAD_GATEWAY, Bad Gateway )                                                                              \
  XX( 503, SERVICE_UNAVAILABLE, Service Unavailable )                                                              \
  XX( 504, GATEWAY_TIMEOUT, Gateway Timeout )                                                                      \
  XX( 505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported )                                                \
  XX( 506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates )                                                      \
  XX( 507, INSUFFICIENT_STORAGE, Insufficient Storage )                                                            \
  XX( 508, LOOP_DETECTED, Loop Detected )                                                                          \
  XX( 510, NOT_EXTENDED, Not Extended )                                                                            \
  XX( 511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required )

enum class HttpMethod
{
#define XX( num, name, string ) name = num,
  HTTP_METHOD_MAP( XX )
#undef XX
    INVALID_METHOD
};

enum class HttpStatus
{
#define XX( code, name, desc ) name = code,
  HTTP_STATUS_MAP( XX )
#undef XX
};

HttpMethod StringToHttpMethod( const std::string& method );
HttpMethod CharsToHttpMethod( const char* method );
const char* HttpMethodString( const HttpMethod& method );
const char* HttpStatusToString( const HttpStatus& status );

struct CaseInsensitiveLess
{
  bool operator()( const std::string& lhs, const std::string& rhs ) const;
};

template<typename MapType, typename T>
bool checkGetAs( const MapType& mp, const std::string& key, T& val, const T& def = T {} )
{
  auto it = mp.find( key );
  if ( it == mp.end() ) {
    val = def;
    return false;
  }

  try {
    val = boost::lexical_cast<T>( it->second );
    return true;
  } catch ( ... ) {
    val = def;
  }

  return false;
}

template<typename MapType, typename T>
T getAs( const MapType& mp, const std::string& key, const T& def = T() )
{
  auto it = mp.find( key );
  if ( it == mp.end() ) {
    return def;
  }

  try {
    return boost::lexical_cast<T>( it->second );
  } catch ( ... ) {}

  return def;
}

class HttpRequest
{
public:
  using SPtr = std::shared_ptr<HttpRequest>;
  using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

  HttpRequest( uint8_t version = 0x11, bool close = true );

  HttpMethod getMethod() const { return m_method; }
  uint8_t getVersion() const { return m_version; }
  const std::string& getPath() const { return m_path; }
  const std::string& getQuery() const { return m_query; }
  const std::string& getBody() const { return m_body; }
  const MapType& getHeaders() const { return m_headers; }
  const MapType& getParams() const { return m_params; }
  const MapType& getCookies() const { return m_cookies; }

  void setMethod( HttpMethod method ) { m_method = method; }
  void setVersion( uint8_t version ) { m_version = version; }
  void setPath( const std::string& path ) { m_path = path; }
  void setQuery( const std::string& query ) { m_query = query; }
  void setFragment( const std::string& fragment ) { m_fragment = fragment; }
  void setBody( const std::string& body ) { m_body = body; }
  void appendBody( const std::string& val ) { m_body.append( val ); }

  bool isClose() const { return m_close; }
  void setClose( bool val ) { m_close = val; }

  void setHeaders( const MapType& val ) { m_headers = val; }
  void setParams( const MapType& val ) { m_params = val; }
  void setCookie( const MapType& val ) { m_cookies = val; }

  std::string getHeader( const std::string& key, const std::string& def = "" ) const;
  std::string getParam( const std::string& key, const std::string& def = "" ) const;
  std::string getCookie( const std::string& key, const std::string& def = "" ) const;

  void setHeader( const std::string& key, const std::string& val );
  void setParam( const std::string& key, const std::string& val );
  void setCookie( const std::string& key, const std::string& val );

  void delHeader( const std::string& key );
  void delParam( const std::string& key );
  void delCookie( const std::string& key );

  bool hasHeader( const std::string& key, std::string& val );
  bool hasParam( const std::string& key, std::string& val );
  bool hasCookie( const std::string& key, std::string& val );

  template<typename T>
  bool checkGetHeaderAs( const std::string& key, T& val, const T& def = T {} )
  {
    return checkGetAs( m_headers, key, val, def );
  }

  template<typename T>
  T getHeader( const std::string& key, T& val, const T& def = T {} )
  {
    return getAs( m_headers, key, def );
  }

  template<typename T>
  bool checkGetParamAs( const std::string& key, T& val, const T& def = T {} )
  {
    return checkGetAs( m_headers, key, val, def );
  }

  template<typename T>
  T getParamAs( const std::string& key, const T& def = T {} )
  {
    return getAs( m_headers, key, def );
  }

  template<typename T>
  bool checkGetCookieAs( const std::string& key, T& val, const T& def = T {} )
  {
    return checkGetAs( m_headers, key, val, def );
  }

  template<typename T>
  T getCookieAs( const std::string& key, const T& def = T {} )
  {
    return getAs( m_headers, key, def );
  }

  std::ostream& dump( std::ostream& os ) const;
  std::string toString() const;

private:
  HttpMethod m_method;
  uint8_t m_version;
  bool m_close;

  std::string m_path;
  std::string m_query;
  std::string m_fragment;
  std::string m_body;

  MapType m_headers;
  MapType m_params;
  MapType m_cookies;
};

class HttpResponse
{
public:
  using SPtr = std::shared_ptr<HttpResponse>;
  using MapType = std::map<std::string, std::string, CaseInsensitiveLess>;

  HttpResponse( uint8_t version = 0x11, bool close = true );

  HttpStatus getStatus() const { return m_status; }
  uint8_t getVersion() const { return m_version; }
  const std::string& getBody() const { return m_body; }
  const std::string& getReason() const { return m_reason; }
  const MapType& getHeaders() const { return m_headers; }

  void setStatus( HttpStatus status ) { m_status = status; }
  void setVersion( uint8_t version ) { m_version = version; }
  void setBody( const std::string& body ) { m_body = body; }
  void appendBody( const std::string& body ) { m_body.append( body ); }
  void setReason( const std::string& reason ) { m_reason = reason; }
  void setHeaders( const MapType& val ) { m_headers = val; }

  bool isClose() const { return m_close; }
  void setClose( bool val ) { m_close = val; }

  std::string getHeader( const std::string& key, const std::string& def = "" ) const;
  void setHeader( const std::string& key, const std::string& val );
  void delHeader( const std::string& key );

  template<typename T>
  bool checkGetHeaderAs( const std::string& key, const std::string& def = "" ) const;

  template<typename T>
  T getHeaderAs( const std::string& key, T& val, const T& def = T {} )
  {
    return checkGetAs( m_headers, key, val, def );
  }

  std::ostream& dump( std::ostream& os ) const;
  std::string toString() const;

private:
  HttpStatus m_status;
  uint8_t m_version;
  bool m_close;
  std::string m_body;
  std::string m_reason;
  MapType m_headers;
};

}

}