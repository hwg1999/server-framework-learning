#include "sylar/uri.h"
#include "sylar/address.h"
#include "sylar/http/http-parser/http_parser.h"
#include <cmath>
#include <memory>
#include <sstream>
#include <string>

namespace sylar {

Uri::SPtr Uri::Create( const std::string& urlstr )
{
  Uri::SPtr uri { std::make_shared<Uri>() };
  struct http_parser_url parser;
  http_parser_url_init( &parser );

  if ( int ret = http_parser_parse_url( urlstr.c_str(), urlstr.length(), 0, &parser ); ret != 0 ) {
    return nullptr;
  }

  if ( parser.field_set & ( 1 << UF_SCHEMA ) ) {
    uri->setScheme(
      std::string( urlstr.c_str() + parser.field_data[UF_SCHEMA].off, parser.field_data[UF_SCHEMA].len ) );
  }

  if ( parser.field_set & ( 1 << UF_USERINFO ) ) {
    uri->setUserInfo(
      std::string( urlstr.c_str(), +parser.field_data[UF_USERINFO].off, parser.field_data[UF_USERINFO].len ) );
  }

  if ( parser.field_set & ( 1 << UF_HOST ) ) {
    uri->setHost( std::string( urlstr.c_str() + parser.field_data[UF_HOST].off, parser.field_data[UF_HOST].len ) );
  }

  if ( parser.field_set & ( 1 << UF_PORT ) ) {
    uri->setPort(
      std::stoi( std::string( urlstr.c_str() + parser.field_data[UF_PORT].off, parser.field_data[UF_PORT].len ) ) );
  } else {
    // 默认端口号解析只支持http/ws/https
    if ( uri->getScheme() == "http" || uri->getScheme() == "ws" ) {
      uri->setPort( 80 );
    } else if ( uri->getScheme() == "https" ) {
      uri->setPort( 443 );
    }
  }

  if ( parser.field_set & ( 1 << UF_PATH ) ) {
    uri->setPath( std::string( urlstr.c_str() + parser.field_data[UF_PATH].off, parser.field_data[UF_PATH].len ) );
  }

  if ( parser.field_set & ( 1 << UF_QUERY ) ) {
    uri->setQuery(
      std::string( urlstr.c_str() + parser.field_data[UF_QUERY].off, parser.field_data[UF_QUERY].len ) );
  }

  if ( parser.field_set & ( 1 << UF_FRAGMENT ) ) {
    uri->setFragment(
      std::string( urlstr.c_str() + parser.field_data[UF_FRAGMENT].off, parser.field_data[UF_FRAGMENT].len ) );
  }

  return uri;
}

Uri::Uri() : port_ { 0 } {}

int32_t Uri::getPort() const
{
  if ( port_ ) {
    return port_;
  }

  if ( scheme_ == "http" || scheme_ == "ws" ) {
    return 80;
  } else if ( scheme_ == "https" || scheme_ == "wss" ) {
    return 443;
  }

  return port_;
}

const std::string& Uri::getPath() const
{
  static std::string s_default_path = "/";
  return path_.empty() ? s_default_path : path_;
}

bool Uri::isDefaultPort() const
{
  if ( scheme_ == "http" || scheme_ == "ws" ) {
    return port_ == 80;
  } else if ( scheme_ == "https" ) {
    return port_ == 443;
  }

  return false;
}

std::string Uri::toString() const
{
  std::stringstream ss;

  ss << scheme_ << "://" << userInfo_ << ( userInfo_.empty() ? "" : "@" ) << host_
     << ( isDefaultPort() ? "" : ":" + std::to_string( port_ ) ) << getPath() << ( query_.empty() ? "" : "?" )
     << query_ << ( fragment_.empty() ? "" : "#" ) << fragment_;

  return ss.str();
}

Address::SPtr Uri::createAddress() const
{
  auto addr = Address::LookUpAnyIPAddress( host_ );
  if ( addr ) {
    addr->setPort( getPort() );
  }
  return addr;
}

}