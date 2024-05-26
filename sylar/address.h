#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <netinet/in.h>
#include <ostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace sylar {

class IPAddress;

class Address
{
public:
  using SPtr = std::shared_ptr<Address>;

  static SPtr Create( const sockaddr* addr, socklen_t addrlen );
  static bool LookUp( std::vector<SPtr>& result,
                      const std::string& host,
                      int family = AF_UNSPEC,
                      int type = 0,
                      int protocol = 0 );
  static SPtr LoopUpAny( const std::string& host, int family = AF_UNSPEC, int type = 0, int protocol = 0 );
  static std::shared_ptr<IPAddress> LoopUpAnyIPAddress( const std::string& host,
                                                        int family = AF_UNSPEC,
                                                        int type = 0,
                                                        int protocol = 0 );
  static bool GetInterfaceAddress( std::multimap<std::string, std::pair<SPtr, uint32_t>>& result,
                                   int family = AF_UNSPEC );
  static bool GetInterfaceAddress( std::vector<std::pair<SPtr, uint32_t>>& result,
                                   const std::string& iface,
                                   int family = AF_UNSPEC );

  virtual ~Address() {}

  int getFamily() const;

  virtual const sockaddr* getAddr() const = 0;
  virtual socklen_t getAddrLen() const = 0;

  virtual std::ostream& insert( std::ostream& os ) const = 0;
  std::string toString();

  bool operator<( const Address& rhs ) const;
  bool operator==( const Address& rhs ) const;
  bool operator!=( const Address& rhs ) const;
};

class IPAddress : public Address
{
public:
  using SPtr = std::shared_ptr<IPAddress>;

  static SPtr Create( const char* address, uint32_t port = 0 );

  virtual SPtr broadcastAddress( uint32_t prefix_len ) = 0;
  virtual SPtr networdAddress( uint32_t prefix_len ) = 0;
  virtual SPtr subnetMask( uint32_t prefix_len ) = 0;

  virtual uint32_t getPort() const = 0;
  virtual void setPort( uint32_t val ) = 0;
};

class IPv4Address : public IPAddress
{
public:
  using SPtr = std::shared_ptr<IPv4Address>;

  static SPtr Create( const char* address, uint32_t port = 0 );

  IPv4Address( const sockaddr_in& address );
  IPv4Address( uint32_t address = INADDR_ANY, uint32_t port = 0 );

  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& insert( std::ostream& os ) const override;

  IPAddress::SPtr broadcastAddress( uint32_t prefix_len ) override;
  IPAddress::SPtr networdAddress( uint32_t prefix_len ) override;
  IPAddress::SPtr subnetMask( uint32_t prefix_len ) override;
  uint32_t getPort() const override;
  void setPort( uint32_t val ) override;

private:
  sockaddr_in m_addr;
};

class IPv6Address : public IPAddress
{
public:
  using SPtr = std::shared_ptr<IPv6Address>;

  static SPtr Create( const char* address, uint32_t port = 0 );

  IPv6Address();
  IPv6Address( const sockaddr_in6& address );
  IPv6Address( const uint8_t address[16], uint32_t port = 0 );

  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& insert( std::ostream& os ) const override;

  IPAddress::SPtr broadcastAddress( uint32_t prefix_len ) override;
  IPAddress::SPtr networdAddress( uint32_t prefix_len ) override;
  IPAddress::SPtr subnetMask( uint32_t prefix_len ) override;
  uint32_t getPort() const override;
  void setPort( uint32_t val ) override;

private:
  sockaddr_in6 m_addr;
};

class UnixAddress : public Address
{
public:
  using SPtr = std::shared_ptr<UnixAddress>;
  UnixAddress();
  UnixAddress( const std::string& path );

  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& insert( std::ostream& os ) const override;

private:
  sockaddr_un m_addr;
  socklen_t m_length;
};

class UnknownAddress : public Address
{
public:
  using SPtr = std::shared_ptr<UnknownAddress>;
  UnknownAddress( int family );
  UnknownAddress( const sockaddr& addr );
  const sockaddr* getAddr() const override;
  socklen_t getAddrLen() const override;
  std::ostream& insert( std::ostream& os ) const override;

private:
  sockaddr m_addr;
};

}