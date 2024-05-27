#pragma once

#include "address.h"
#include "noncopyable.h"
#include <cstddef>
#include <memory>
#include <ostream>
#include <sys/socket.h>

namespace sylar {

class Socket
  : public std::enable_shared_from_this<Socket>
  , Noncopyable
{
public:
  using SPtr = std::shared_ptr<Socket>;
  using WPtr = std::weak_ptr<Socket>;

  enum Type
  {
    TCP = SOCK_STREAM,
    UDP = SOCK_DGRAM,
  };

  enum Family
  {
    IPv4 = AF_INET,
    IPv6 = AF_INET6,
    UNIX = AF_UNIX
  };

  static SPtr CreateTCP( sylar::Address::SPtr address );
  static SPtr CreateUDP( sylar::Address::SPtr address );

  static SPtr CreateTCPSocket();
  static SPtr CreateUDPSocket();

  static SPtr CreateTCPSocket6();
  static SPtr CreateUDPSocket6();

  static SPtr CreateUnixTCPSocket();
  static SPtr CreateUnixUDPSocket();

  Socket( int family, int type, int protocol = 0 );
  ~Socket();

  int64_t getSendTimeout();
  void setSendTimeout( int64_t val );

  int64_t getRecvTimeout();
  void setRecvTimeout( int64_t val );

  bool getOption( int level, int option, void* result, size_t* len );

  template<typename T>
  bool getOption( int level, int option, T& result )
  {
    size_t length = sizeof( T );
    return getOption( level, option, &result, &length );
  }

  bool setOption( int level, int option, const void* result, size_t len );

  template<typename T>
  bool setOption( int level, int option, const T& value )
  {
    return setOption( level, option, &value, sizeof( T ) );
  }

  SPtr accept();

  bool bind( const Address::SPtr& addr );
  bool connect( const Address::SPtr& addr, uint64_t timeout_ms = -1 );
  bool listen( int backlog = SOMAXCONN );
  bool close();

  int send( const void* buffer, size_t length, int flags = 0 );
  int send( const iovec* buffers, size_t length, int flags = 0 );
  int sendTo( const void* buffer, size_t length, const Address::SPtr to, int flags = 0 );
  int sendTo( const iovec* buffers, size_t length, const Address::SPtr to, int flags = 0 );

  int recv( void* buffer, size_t length, int flags = 0 );
  int recv( iovec* buffers, size_t length, int flags = 0 );
  int recvFrom( void* buffer, size_t length, Address::SPtr from, int flags = 0 );
  int recvFrom( iovec* buffers, size_t length, Address::SPtr from, int flags = 0 );

  Address::SPtr getRemoteAddress();
  Address::SPtr getLocalAddress();

  int getFamily() const { return m_family; }
  int getType() const { return m_type; }
  int getProtocol() const { return m_protocol; }

  bool isConnected() const { return m_isConnected; }
  bool isValid() const;
  int getError();

  std::ostream& dump( std::ostream& os ) const;
  int getSocket() const { return m_sock; }

  bool cancelRead();
  bool cancelWrite();
  bool cancelAccept();
  bool cancelAll();

private:
  void initSock();
  void newSock();
  bool init( int sock );

private:
  int m_sock;
  int m_family;
  int m_type;
  int m_protocol;
  int m_isConnected;

  Address::SPtr m_localAddress;
  Address::SPtr m_remoteAddress;
};

}