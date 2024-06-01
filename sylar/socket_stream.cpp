#include "sylar/socket_stream.h"
#include "sylar/bytearray.h"
#include "sylar/socket.h"
#include <bits/types/struct_iovec.h>
#include <vector>

namespace sylar {

SocketStream::SocketStream( Socket::SPtr sock, bool owner ) : m_socket { sock }, m_owner { owner } {}

SocketStream::~SocketStream()
{
  if ( m_owner && m_socket ) {
    m_socket->close();
  }
}

bool SocketStream::isConnected() const
{
  return m_socket && m_socket->isConnected();
}

int SocketStream::read( void* buffer, size_t length )
{
  if ( !isConnected() ) {
    return -1;
  }
  return m_socket->recv( buffer, length );
}

int SocketStream::read( ByteArray::SPtr ba, size_t length )
{
  if ( !isConnected() ) {
    return -1;
  }

  std::vector<iovec> iovs;
  ba->getWriteBuffers( iovs, length );
  int ret = m_socket->recv( iovs.data(), iovs.size() );
  if ( ret > 0 ) {
    ba->setPosition( ba->getPosition() + ret );
  }

  return ret;
}

int SocketStream::write( const void* buffer, size_t length )
{
  if ( !isConnected() ) {
    return -1;
  }
  return m_socket->send( buffer, length );
}

int SocketStream::write( ByteArray::SPtr ba, size_t length )
{
  if ( !isConnected() ) {
    return -1;
  }

  std::vector<iovec> iovs;
  ba->getReadBuffers( iovs, length );
  int ret = m_socket->send( iovs.data(), iovs.size() );
  if ( ret > 0 ) {
    ba->setPosition( ba->getPosition() + ret );
  }

  return ret;
}

void SocketStream::close()
{
  if ( m_socket ) {
    m_socket->close();
  }
}

}