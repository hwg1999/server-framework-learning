#include "socket.h"
#include "sylar/address.h"
#include "sylar/fd_manager.h"
#include "sylar/hook.h"
#include "sylar/iomanager.h"
#include "sylar/log.h"
#include <asm-generic/socket.h>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <ostream>
#include <sys/socket.h>
#include <unistd.h>

namespace sylar {

static sylar::Logger::SPtr g_logger = SYLAR_LOG_NAME( "system" );

Socket::SPtr Socket::CreateTCP( sylar::Address::SPtr address )
{
  return std::make_shared<Socket>( address->getFamily(), TCP, 0 );
}

Socket::SPtr Socket::CreateUDP( sylar::Address::SPtr address )
{
  return std::make_shared<Socket>( address->getFamily(), UDP, 0 );
}

Socket::SPtr Socket::CreateTCPSocket()
{
  return std::make_shared<Socket>( IPv4, TCP, 0 );
}

Socket::SPtr Socket::CreateUDPSocket()
{
  return std::make_shared<Socket>( IPv4, UDP, 0 );
}

Socket::SPtr Socket::CreateTCPSocket6()
{
  return std::make_shared<Socket>( IPv6, TCP, 0 );
}

Socket::SPtr Socket::CreateUDPSocket6()
{
  return std::make_shared<Socket>( IPv6, UDP, 0 );
}

Socket::SPtr Socket::CreateUnixTCPSocket()
{
  return std::make_shared<Socket>( UNIX, TCP, 0 );
}

Socket::SPtr Socket::CreateUnixUDPSocket()
{
  return std::make_shared<Socket>( UNIX, UDP, 0 );
}

Socket::Socket( int family, int type, int protocol )
  : m_sock( -1 ), m_family { family }, m_type { type }, m_protocol { protocol }, m_isConnected { false }
{}

Socket::~Socket()
{
  ::close( m_sock );
}

int64_t Socket::getSendTimeout()
{
  FdCtx::SPtr ctx = FdMgr::GetInstance().get( m_sock );
  if ( ctx ) {
    return ctx->getTimeout( SO_SNDTIMEO );
  }
  return -1;
}

void Socket::setSendTimeout( int64_t val )
{
  struct timeval tv
  {
    int( val / 1000 ), int( val % 1000 * 1000 )
  };
  setOption( SOL_SOCKET, SO_RCVTIMEO, tv );
}

bool Socket::getOption( int level, int option, void* result, size_t* len )
{
  int ret = getsockopt( m_sock, level, option, result, (socklen_t*)len );
  if ( ret ) {
    SYLAR_LOG_DEBUG( g_logger ) << "setOption sock=" << m_sock << " level=" << level << " option=" << option
                                << " errno=" << errno << " errstr=" << strerror( errno );
    return false;
  }
  return true;
}

bool Socket::setOption( int level, int option, const void* result, size_t len )
{
  int ret = setsockopt( m_sock, level, option, result, (socklen_t)len );
  if ( ret ) {
    SYLAR_LOG_ERROR( g_logger ) << "setOption sock=" << m_sock << " level=" << level << " option=" << option
                                << " errno=" << errno << " errstr=" << strerror( errno );
    return false;
  }
  return true;
}

Socket::SPtr Socket::accept()
{
  Socket::SPtr sock( std::make_shared<Socket>( m_family, m_type, m_protocol ) );
  int newSock = ::accept( m_sock, nullptr, nullptr );
  if ( -1 == newSock ) {
    SYLAR_LOG_ERROR( g_logger ) << "accept(" << m_sock << ") errno=" << errno << " errstr=" << strerror( errno );
    return nullptr;
  }

  if ( sock->init( newSock ) ) {
    return sock;
  }
  return nullptr;
}

bool Socket::init( int sock )
{
  FdCtx::SPtr ctx = FdMgr::GetInstance().get( sock );
  if ( ctx && ctx->isSocket() && !ctx->isClose() ) {
    m_sock = sock;
    m_isConnected = true;
    initSock();
    getLocalAddress();
    getRemoteAddress();
    return true;
  }
  return false;
}

bool Socket::bind( const Address::SPtr& addr )
{
  if ( !isValid() ) {
    newSock();
    if ( !isValid() ) {
      return false;
    }
  }

  if ( addr->getFamily() != m_family ) {
    SYLAR_LOG_DEBUG( g_logger ) << "bind sock.family(" << m_family << ") addr.family(" << addr->getFamily()
                                << ") not equal, addr=" << addr->toString();
    return false;
  }

  if ( ::bind( m_sock, addr->getAddr(), addr->getAddrLen() ) ) {
    SYLAR_LOG_ERROR( g_logger ) << "bind error, errno = " << errno << " errstr=" << strerror( errno );
    return false;
  }

  getLocalAddress();
  return true;
}

bool Socket::connect( const Address::SPtr& addr, uint64_t timeout_ms )
{
  if ( !isValid() ) {
    newSock();
    if ( !isValid() ) {
      return false;
    }
  }

  if ( addr->getFamily() != m_family ) {
    SYLAR_LOG_ERROR( g_logger ) << "connect sock.family(" << m_family << ") addr.family(" << addr->getFamily()
                                << ") not equal, addr = " << addr->toString();
    return false;
  }

  if ( timeout_ms == -1 ) {
    if ( int ret = ::connect( m_sock, addr->getAddr(), addr->getAddrLen() ); ret ) {
      SYLAR_LOG_ERROR( g_logger ) << "sock=" << m_sock << " connect(" << addr->toString()
                                  << ") error, errno = " << errno << " errstr=" << strerror( errno );
      close();
      return false;
    }
  } else {
    if ( int ret = ::connect_with_timeout( m_sock, addr->getAddr(), addr->getAddrLen(), timeout_ms ); ret ) {
      SYLAR_LOG_ERROR( g_logger ) << "sock=" << m_sock << " connect(" << addr->toString()
                                  << ") timeout=" << timeout_ms << "error, errno=" << errno
                                  << " errstr=" << strerror( errno );
      close();
      return false;
    }
  }

  m_isConnected = true;
  getRemoteAddress();
  getLocalAddress();

  return true;
}

bool Socket::listen( int backlog )
{
  if ( !isValid() ) {
    SYLAR_LOG_ERROR( g_logger ) << "listen error sock=-1";
    return false;
  }

  if ( int ret = ::listen( m_sock, backlog ); ret ) {
    SYLAR_LOG_ERROR( g_logger ) << "listen error, errno=" << errno << " errstr=" << strerror( errno );
    return false;
  }

  return true;
}

bool Socket::close()
{
  if ( !m_isConnected && m_sock == -1 ) {
    return true;
  }

  m_isConnected = false;
  if ( m_sock != -1 ) {
    ::close( m_sock );
    m_sock = -1;
  }

  return false;
}

int Socket::send( const void* buffer, size_t length, int flags )
{
  if ( isConnected() ) {
    return ::send( m_sock, buffer, length, flags );
  }

  return -1;
}

int Socket::send( const iovec* buffers, size_t length, int flags )
{
  if ( isConnected() ) {
    msghdr msg;
    std::memset( &msg, 0, sizeof( msg ) );
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    return ::sendmsg( m_sock, &msg, flags );
  }

  return -1;
}

int Socket::sendTo( const void* buffer, size_t length, const Address::SPtr to, int flags )
{
  if ( isConnected() ) {
    return ::sendto( m_sock, buffer, length, flags, to->getAddr(), to->getAddrLen() );
  }

  return -1;
}

int Socket::sendTo( const iovec* buffers, size_t length, const Address::SPtr to, int flags )
{
  if ( isConnected() ) {
    msghdr msg;
    std::memset( &msg, 0, sizeof( msg ) );
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    msg.msg_name = (char*)to->getAddr();
    msg.msg_namelen = to->getAddrLen();
    return ::sendmsg( m_sock, &msg, flags );
  }

  return -1;
}

int Socket::recv( void* buffer, size_t length, int flags )
{
  if ( isConnected() ) {
    return ::recv( m_sock, buffer, length, flags );
  }

  return -1;
}

int Socket::recv( iovec* buffers, size_t length, int flags )
{
  if ( isConnected() ) {
    msghdr msg;
    std::memset( &msg, 0, sizeof( msg ) );
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    return ::recvmsg( m_sock, &msg, flags );
  }

  return -1;
}

int Socket::recvFrom( void* buffer, size_t length, Address::SPtr from, int flags )
{
  if ( isConnected() ) {
    socklen_t len = from->getAddrLen();
    return ::recvfrom( m_sock, buffer, length, flags, (struct sockaddr*)from->getAddr(), &len );
  }
  return -1;
}

int Socket::recvFrom( iovec* buffers, size_t length, Address::SPtr from, int flags )
{
  if ( isConnected() ) {
    msghdr msg;
    memset( &msg, 0, sizeof( msg ) );
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    msg.msg_name = (char*)from->getAddr();
    msg.msg_namelen = from->getAddrLen();
    return ::recvmsg( m_sock, &msg, flags );
  }

  return -1;
}

Address::SPtr Socket::getRemoteAddress()
{
  if ( m_remoteAddress ) {
    return m_remoteAddress;
  }

  Address::SPtr result;
  switch ( m_family ) {
    case AF_INET:
      result.reset( new IPv4Address );
      break;
    case AF_INET6:
      result.reset( new IPv6Address );
      break;
    case AF_UNIX:
      result.reset( new UnixAddress );
      break;
    default:
      result.reset( new UnknownAddress( m_family ) );
      break;
  }

  socklen_t addrlen = result->getAddrLen();
  if ( int ret = getpeername( m_sock, (struct sockaddr*)result->getAddr(), &addrlen ); ret ) {
    SYLAR_LOG_ERROR( g_logger ) << "getpeername error sock=" << m_sock << " errno=" << errno
                                << " errstr=" << strerror( errno );
    return std::make_shared<UnknownAddress>( m_family );
  }

  if ( m_family == AF_UNIX ) {
    UnixAddress::SPtr addr = std::dynamic_pointer_cast<UnixAddress>( result );
    addr->setAddrLen( addrlen );
  }

  m_remoteAddress = result;
  return m_remoteAddress;
}

Address::SPtr Socket::getLocalAddress()
{
  if ( m_localAddress ) {
    return m_localAddress;
  }

  Address::SPtr result;
  switch ( m_family ) {
    case AF_INET:
      result.reset( new IPv4Address );
      break;
    case AF_INET6:
      result.reset( new IPv6Address );
      break;
    case AF_UNIX:
      result.reset( new UnixAddress );
      break;
    default:
      result.reset( new UnknownAddress( m_family ) );
      break;
  }

  socklen_t addrlen = result->getAddrLen();
  if ( int ret = getsockname( m_sock, result->getAddr(), &addrlen ); ret ) {
    SYLAR_LOG_ERROR( g_logger ) << "getsockname error, sock=" << m_sock << " errno=" << errno
                                << " errstr=" << strerror( errno );
    return std::make_shared<UnknownAddress>( m_family );
  }

  if ( m_family == AF_UNIX ) {
    UnixAddress::SPtr addr = std::dynamic_pointer_cast<UnixAddress>( result );
    addr->setAddrLen( addrlen );
  }

  m_localAddress = result;
  return m_localAddress;
}

bool Socket::isValid() const
{
  return m_sock != -1;
}

int Socket::getError()
{
  int error = 0;
  size_t len = sizeof( error );
  if ( int ret = getOption( SOL_SOCKET, SO_ERROR, &error, &len ); ret ) {
    return -1;
  }
  return error;
}

std::ostream& Socket::dump( std::ostream& os ) const
{
  os << "[Socket sock=" << m_sock << " is_connected=" << m_isConnected << " family=" << m_family
     << " type=" << m_type << " protocol=" << m_protocol;
  if ( m_localAddress ) {
    os << " local_address=" << m_localAddress->toString();
  }
  if ( m_remoteAddress ) {
    os << " remote_address=" << m_remoteAddress->toString();
  }
  os << "]";
  return os;
}

bool Socket::cancelRead()
{
  return IOManager::GetThis()->cancelEvent( m_sock, sylar::IOManager::READ );
}

bool Socket::cancelWrite()
{
  return IOManager::GetThis()->cancelEvent( m_sock, sylar::IOManager::WRITE );
}

bool Socket::cancelAccept()
{
  return IOManager::GetThis()->cancelEvent( m_sock, sylar::IOManager::READ );
}

bool Socket::cancelAll()
{
  return IOManager::GetThis()->cancelAll( m_sock );
}

void Socket::initSock()
{
  int val = 1;
  setOption( SOL_SOCKET, SO_REUSEADDR, val );
  if ( m_type == SOCK_STREAM ) {
    setOption( IPPROTO_TCP, TCP_NODELAY, val );
  }
}

void Socket::newSock()
{
  m_sock = socket( m_family, m_type, m_protocol );
  if ( m_sock != -1 ) {
    initSock();
  } else {
    SYLAR_LOG_ERROR( g_logger ) << "socket(" << m_family << ", " << m_type << ", " << m_protocol
                                << ") errno=" << errno << " errstr=" << strerror( errno );
  }
}

}