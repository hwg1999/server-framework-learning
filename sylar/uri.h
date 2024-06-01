#pragma once

#include "sylar/address.h"
#include <memory>
#include <ostream>
#include <string>

namespace sylar {

class Uri
{
public:
  using SPtr = std::shared_ptr<Uri>;

  static SPtr Create( const std::string& uri );

  Uri();

  const std::string& getScheme() const { return scheme_; }
  const std::string& getUserInfo() const { return userInfo_; }
  const std::string& getHost() const { return host_; }
  const std::string& getPath() const;
  const std::string& getQuery() const { return query_; }
  const std::string& getFragment() const { return fragment_; }
  int32_t getPort() const;

  void setScheme( const std::string& scheme ) { scheme_ = scheme; }
  void setUserInfo( const std::string& userInfo ) { userInfo_ = userInfo; }
  void setHost( const std::string& host ) { host_ = host; }
  void setPath( const std::string& path ) { path_ = path; }
  void setQuery( const std::string& query ) { query_ = query; }
  void setFragment( const std::string& fragment ) { fragment_ = fragment; }
  void setPort( uint32_t port ) { port_ = port; }

  std::ostream& dump( std::ostream& os ) const;
  std::string toString() const;
  Address::SPtr createAddress() const;

private:
  bool isDefaultPort() const;

private:
  std::string scheme_;
  std::string userInfo_;
  std::string host_;
  std::string path_;
  std::string query_;
  std::string fragment_;
  int32_t port_;
};

}