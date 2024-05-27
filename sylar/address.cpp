#include "address.h"
#include "sylar/endian.h"
#include "sylar/log.h"
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ifaddrs.h>
#include <limits>
#include <map>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <utility>
#include <vector>

namespace sylar {

static sylar::Logger::SPtr g_logger = SYLAR_LOG_ROOT();

template<typename T>
static T CreateMask( uint32_t bits )
{
  return ( 1 << ( sizeof( T ) * 8 - bits ) ) - 1;
}

template<typename T>
static uint32_t CountBytes( T value )
{
  uint32_t result = 0;
  for ( ; value; ++result ) {
    value &= value - 1;
  }
  return result;
}

Address::SPtr Address::LoopUpAny( const std::string& host, int family, int type, int protocol )
{
  std::vector<Address::SPtr> result;
  if ( LookUp( result, host, family, type, protocol ) ) {
    return result[0];
  }
  return nullptr;
}

IPAddress::SPtr Address::LoopUpAnyIPAddress( const std::string& host, int family, int type, int protocol )
{
  std::vector<Address::SPtr> result;
  if ( LookUp( result, host, family, type, protocol ) ) {
    for ( auto& res : result ) {
      IPAddress::SPtr val = std::dynamic_pointer_cast<IPAddress>( res );
      if ( val ) {
        return val;
      }
    }
  }

  return nullptr;
}

bool Address::LookUp( std::vector<SPtr>& result, const std::string& host, int family, int type, int protocol )
{
  addrinfo hints;
  hints.ai_flags = 0;
  hints.ai_family = family;
  hints.ai_socktype = type;
  hints.ai_protocol = protocol;
  hints.ai_addrlen = 0;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  std::string node;
  const char* service = nullptr;
  if ( !host.empty() && host[0] == '[' ) {
    const char* endipv6 = (const char*)memchr( host.c_str() + 1, ']', host.size() - 1 );
    if ( endipv6 ) {
      if ( *( endipv6 + 1 ) == ':' ) {
        service = endipv6 + 2;
      }
      node = host.substr( 1, endipv6 - host.c_str() - 1 );
    }
  }

  if ( node.empty() ) {
    service = (const char*)memchr( host.c_str(), ':', host.size() );
    if ( service ) {
      if ( !memchr( service + 1, ':', host.c_str() + host.size() - service - 1 ) ) {
        node = host.substr( 0, service - host.c_str() );
        ++service;
      }
    }
  }

  if ( node.empty() ) {
    node = host;
  }

  addrinfo* results;
  int error = getaddrinfo( node.c_str(), service, &hints, &results );
  if ( error ) {
    SYLAR_LOG_ERROR( g_logger ) << "Address::Lookup getaddress(" << host << ", " << family << ", " << type
                                << ") err=" << error << " errstr = " << strerror( errno );
    return false;
  }

  addrinfo* next = results;
  while ( next ) {
    result.push_back( Create( next->ai_addr, (socklen_t)next->ai_addrlen ) );
    next = next->ai_next;
  }

  freeaddrinfo( results );
  return true;
}

bool Address::GetInterfaceAddress( std::multimap<std::string, std::pair<Address::SPtr, uint32_t>>& result,
                                   int family )
{
  struct ifaddrs* results;
  if ( getifaddrs( &results ) != 0 ) {
    SYLAR_LOG_ERROR( g_logger ) << "Address::GetInterfaceAddresses getifaddrs err=" << errno
                                << " errstr=" << strerror( errno );
    return false;
  }

  try {
    for ( struct ifaddrs* next = results; next; next = next->ifa_next ) {
      Address::SPtr addr;
      uint32_t prefix_len = std::numeric_limits<uint32_t>::max();
      if ( family != AF_UNSPEC && family != next->ifa_addr->sa_family ) {
        continue;
      }

      switch ( next->ifa_addr->sa_family ) {
        case AF_INET: {
          addr = Create( next->ifa_addr, sizeof( sockaddr_in ) );
          uint32_t netmask = ( (sockaddr_in*)next->ifa_netmask )->sin_addr.s_addr;
          prefix_len = CountBytes( netmask );
          break;
        }
        case AF_INET6: {
          addr = Create( next->ifa_addr, sizeof( sockaddr_in6 ) );
          in6_addr& netmask = ( (sockaddr_in6*)next->ifa_netmask )->sin6_addr;
          prefix_len = 0;
          for ( int i = 0; i < 16; ++i ) {
            prefix_len += CountBytes( netmask.s6_addr[i] );
          }
          break;
        }
        default:
          break;
      }

      if ( addr ) {
        result.insert( std::make_pair( next->ifa_name, std::make_pair( addr, prefix_len ) ) );
      }
    }

  } catch ( ... ) {
    SYLAR_LOG_ERROR( g_logger ) << "Address::GetInterfaceAddresses exception";
    freeifaddrs( results );
    return false;
  }

  freeifaddrs( results );
  return true;
}

bool Address::GetInterfaceAddress( std::vector<std::pair<Address::SPtr, uint32_t>>& result,
                                   const std::string& iface,
                                   int family )
{
  if ( iface.empty() || iface == "*" ) {
    if ( family == AF_INET || family == AF_UNSPEC ) {
      result.push_back( std::make_pair( Address::SPtr( new IPv4Address() ), 0u ) );
    }
    if ( family == AF_INET6 || family == AF_UNSPEC ) {
      result.push_back( std::make_pair( Address::SPtr( new IPv6Address() ), 0u ) );
    }
    return true;
  }

  std::multimap<std::string, std::pair<Address::SPtr, uint32_t>> results;
  if ( !GetInterfaceAddress( results, family ) ) {
    return false;
  }

  auto its = results.equal_range( iface );
  for ( ; its.first != its.second; ++its.first ) {
    result.push_back( its.first->second );
  }
  return true;
}

int Address::getFamily() const
{
  return getAddr()->sa_family;
}

std::string Address::toString()
{
  std::stringstream ss;
  insert( ss );
  return ss.str();
}

Address::SPtr Address::Create( const sockaddr* addr, socklen_t addrlen )
{
  if ( !addr ) {
    return nullptr;
  }

  Address::SPtr result;
  switch ( addr->sa_family ) {
    case AF_INET:
      result.reset( new IPv4Address( *(const sockaddr_in*)addr ) );
      break;
    case AF_INET6:
      result.reset( new IPv6Address( *(const sockaddr_in6*)addr ) );
      break;
    default:
      break;
  }

  return result;
}

bool Address::operator<( const Address& rhs ) const
{
  socklen_t minLen = std::min( getAddrLen(), rhs.getAddrLen() );
  int result = std::memcmp( getAddr(), rhs.getAddr(), minLen );
  if ( result < 0 ) {
    return true;
  } else if ( result > 0 ) {
    return false;
  } else if ( getAddrLen() < rhs.getAddrLen() ) {
    return true;
  }
  return false;
}

bool Address::operator==( const Address& rhs ) const
{
  return getAddrLen() == rhs.getAddrLen() && std::memcmp( getAddr(), rhs.getAddr(), getAddrLen() ) == 0;
}

bool Address::operator!=( const Address& rhs ) const
{
  return !( *this == rhs );
}

IPAddress::SPtr IPAddress::Create( const char* address, uint32_t port )
{
  addrinfo hints;
  std::memset( &hints, 0, sizeof( addrinfo ) );
  hints.ai_flags = AI_NUMERICHOST;
  hints.ai_family = AF_UNSPEC;

  addrinfo* results;
  int error = getaddrinfo( address, nullptr, &hints, &results );
  if ( error ) {
    SYLAR_LOG_ERROR( g_logger ) << "IPAddress::Create(" << address << ", " << port << ") error=" << error
                                << " errno=" << errno << " errstr=" << strerror( errno );
    return nullptr;
  }

  try {
    IPAddress::SPtr result
      = std::dynamic_pointer_cast<IPAddress>( Address::Create( results->ai_addr, (socklen_t)results->ai_addrlen ) );
    if ( result ) {
      result->setPort( port );
    }
    freeaddrinfo( results );
    return result;
  } catch ( ... ) {
    freeaddrinfo( results );
    return nullptr;
  }

  return nullptr;
}

IPv4Address::SPtr IPv4Address::Create( const char* address, uint32_t port )
{
  IPv4Address::SPtr rt( new IPv4Address );
  rt->m_addr.sin_port = byteswapOnLittleEndian( port );
  int result = inet_pton( AF_INET, address, &rt->m_addr.sin_addr );
  if ( result <= 0 ) {
    SYLAR_LOG_ERROR( g_logger ) << "IPv4Address::Create(" << address << ", " << port << ") rt=" << result
                                << " errno" << errno << " errstr=" << strerror( errno );
    return nullptr;
  }
  return rt;
}

IPv4Address::IPv4Address( const sockaddr_in& address ) : m_addr { address } {}

IPv4Address::IPv4Address( uint32_t address, uint32_t port )
{
  memset( &m_addr, 0, sizeof( m_addr ) );
  m_addr.sin_family = AF_INET;
  m_addr.sin_port = byteswapOnLittleEndian( port );
  m_addr.sin_addr.s_addr = byteswapOnLittleEndian( address );
}

const sockaddr* IPv4Address::getAddr() const
{
  return (sockaddr*)&m_addr;
}

sockaddr* IPv4Address::getAddr()
{
  return (sockaddr*)&m_addr;
}

socklen_t IPv4Address::getAddrLen() const
{
  return sizeof( m_addr );
}

std::ostream& IPv4Address::insert( std::ostream& os ) const
{
  uint32_t addr = byteswapOnLittleEndian( m_addr.sin_addr.s_addr );
  os << ( ( addr >> 24 ) & 0xff ) << "." << ( ( addr >> 16 ) & 0xff ) << "." << ( ( addr >> 8 ) & 0xff ) << "."
     << ( addr & 0xff );
  os << ":" << byteswapOnLittleEndian( m_addr.sin_port );
  return os;
}

IPAddress::SPtr IPv4Address::broadcastAddress( uint32_t prefix_len )
{
  if ( prefix_len > 32 ) {
    return nullptr;
  }

  sockaddr_in baddr { m_addr };
  baddr.sin_addr.s_addr |= byteswapOnLittleEndian( CreateMask<uint32_t>( prefix_len ) );
  return IPv4Address::SPtr( new IPv4Address( baddr ) );
}

IPAddress::SPtr IPv4Address::networdAddress( uint32_t prefix_len )
{
  if ( prefix_len > 32 ) {
    return nullptr;
  }

  sockaddr_in baddr { m_addr };
  baddr.sin_addr.s_addr &= byteswapOnLittleEndian( CreateMask<uint32_t>( prefix_len ) );
  return IPv4Address::SPtr( new IPv4Address( baddr ) );
}

IPAddress::SPtr IPv4Address::subnetMask( uint32_t prefix_len )
{
  sockaddr_in subnet;
  memset( &subnet, 0, sizeof( subnet ) );
  subnet.sin_family = AF_INET;
  subnet.sin_addr.s_addr = ~byteswapOnLittleEndian( CreateMask<uint32_t>( prefix_len ) );
  return std::make_shared<IPv4Address>( subnet );
}

uint32_t IPv4Address::getPort() const
{
  return byteswapOnLittleEndian( m_addr.sin_port );
}

void IPv4Address::setPort( uint32_t val )
{
  m_addr.sin_port = byteswapOnLittleEndian( val );
}

IPv6Address::SPtr IPv6Address::Create( const char* address, uint32_t port )
{
  IPv6Address::SPtr rt( new IPv6Address );
  rt->m_addr.sin6_port = byteswapOnLittleEndian( port );
  int result = inet_pton( AF_INET6, address, &rt->m_addr.sin6_addr );
  if ( result <= 0 ) {
    SYLAR_LOG_ERROR( g_logger ) << "IPv6Address::Create(" << address << ", " << port << ") rt=" << result
                                << " errno=" << errno << " errstr=" << strerror( errno );
    return nullptr;
  }
  return rt;
}

IPv6Address::IPv6Address()
{
  std::memset( &m_addr, 0, sizeof( m_addr ) );
  m_addr.sin6_family = AF_INET6;
}

IPv6Address::IPv6Address( const sockaddr_in6& address ) : m_addr { address } {}

IPv6Address::IPv6Address( const uint8_t address[16], uint32_t port )
{
  std::memset( &m_addr, 0, sizeof( m_addr ) );
  m_addr.sin6_family = AF_INET6;
  m_addr.sin6_port = byteswapOnLittleEndian( port );
  std::memcpy( &m_addr.sin6_addr.s6_addr, address, 16 );
}

const sockaddr* IPv6Address::getAddr() const
{
  return (sockaddr*)&m_addr;
}

sockaddr* IPv6Address::getAddr()
{
  return (sockaddr*)&m_addr;
}

socklen_t IPv6Address::getAddrLen() const
{
  return sizeof( m_addr );
}

std::ostream& IPv6Address::insert( std::ostream& os ) const
{
  os << "[";
  uint16_t* addr = (uint16_t*)m_addr.sin6_addr.s6_addr;
  bool used_zeros = false;
  for ( size_t i = 0; i < 8; ++i ) {
    if ( addr[i] == 0 && !used_zeros ) {
      continue;
    }
    if ( i && addr[i - 1] == 0 && !used_zeros ) {
      os << ":";
      used_zeros = true;
    }
    if ( i ) {
      os << ":";
    }
    os << std::hex << (int)byteswapOnLittleEndian( addr[i] ) << std::dec;
  }

  if ( !used_zeros && addr[7] == 0 ) {
    os << "::";
  }

  os << "]:" << byteswapOnLittleEndian( m_addr.sin6_port );
  return os;
}

IPAddress::SPtr IPv6Address::broadcastAddress( uint32_t prefix_len )
{
  sockaddr_in6 baddr( m_addr );
  baddr.sin6_addr.s6_addr[prefix_len / 8] |= CreateMask<uint8_t>( prefix_len % 8 );
  for ( int i = prefix_len / 8 + 1; i < 16; ++i ) {
    baddr.sin6_addr.s6_addr[i] = 0xff;
  }
  return IPv6Address::SPtr( new IPv6Address( baddr ) );
}

IPAddress::SPtr IPv6Address::networdAddress( uint32_t prefix_len )
{
  sockaddr_in6 baddr( m_addr );
  baddr.sin6_addr.s6_addr[prefix_len / 8] &= CreateMask<uint8_t>( prefix_len % 8 );
  for ( int i = prefix_len / 8 + 1; i < 16; ++i ) {
    baddr.sin6_addr.s6_addr[i] = 0x00;
  }
  return IPv6Address::SPtr( new IPv6Address( baddr ) );
}

IPAddress::SPtr IPv6Address::subnetMask( uint32_t prefix_len )
{
  sockaddr_in6 subnet;
  memset( &subnet, 0, sizeof( subnet ) );
  subnet.sin6_family = AF_INET6;
  subnet.sin6_addr.s6_addr[prefix_len / 8] = ~CreateMask<uint8_t>( prefix_len % 8 );

  for ( uint32_t i = 0; i < prefix_len / 8; ++i ) {
    subnet.sin6_addr.s6_addr[i] = 0xff;
  }
  return IPv6Address::SPtr( new IPv6Address( subnet ) );
}

uint32_t IPv6Address::getPort() const
{
  return byteswapOnLittleEndian( m_addr.sin6_port );
}

void IPv6Address::setPort( uint32_t v )
{
  m_addr.sin6_port = byteswapOnLittleEndian( v );
}

static const size_t MAX_PATH_LEN = sizeof( ( (sockaddr_un*)0 )->sun_path ) - 1;

UnixAddress::UnixAddress()
{
  std::memset( &m_addr, 0, sizeof( m_addr ) );
  m_addr.sun_family = AF_UNIX;
  m_length = offsetof( sockaddr_un, sun_path ) + MAX_PATH_LEN;
}

UnixAddress::UnixAddress( const std::string& path )
{
  std::memset( &m_addr, 0, sizeof( m_addr ) );
  m_addr.sun_family = AF_UNIX;
  m_length = path.size() + 1;

  if ( !path.empty() && path[0] == '\0' ) {
    --m_length;
  }

  if ( m_length > sizeof( m_addr.sun_path ) ) {
    throw std::logic_error( "path too long" );
  }
  std::memcpy( m_addr.sun_path, path.c_str(), m_length );
  m_length += offsetof( sockaddr_un, sun_path );
}

const sockaddr* UnixAddress::getAddr() const
{
  return (sockaddr*)&m_addr;
}

sockaddr* UnixAddress::getAddr()
{
  return (sockaddr*)&m_addr;
}

socklen_t UnixAddress::getAddrLen() const
{
  return m_length;
}

void UnixAddress::setAddrLen( uint32_t val )
{
  m_length = val;
}

std::ostream& UnixAddress::insert( std::ostream& os ) const
{
  if ( m_length > offsetof( sockaddr_un, sun_path ) && m_addr.sun_path[0] == '\0' ) {
    return os << "\\0" << std::string( m_addr.sun_path + 1, m_length - offsetof( sockaddr_un, sun_path ) - 1 );
  }
  return os << m_addr.sun_path;
}

UnknownAddress::UnknownAddress( int family )
{
  memset( &m_addr, 0, sizeof( m_addr ) );
  m_addr.sa_family = family;
}

UnknownAddress::UnknownAddress( const sockaddr& addr )
{
  m_addr = addr;
}

const sockaddr* UnknownAddress::getAddr() const
{
  return &m_addr;
}

sockaddr* UnknownAddress::getAddr()
{
  return &m_addr;
}

socklen_t UnknownAddress::getAddrLen() const
{
  return sizeof( m_addr );
}

std::ostream& UnknownAddress::insert( std::ostream& os ) const
{
  os << "[UnknownAddress family=" << m_addr.sa_family << "]";
  return os;
}

}