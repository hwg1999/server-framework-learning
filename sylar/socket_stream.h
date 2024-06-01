#pragma once

#include "sylar/bytearray.h"
#include "sylar/socket.h"
#include "sylar/stream.h"
#include <memory>

namespace sylar {

class SocketStream : public Stream
{
public:
  using SPtr = std::shared_ptr<SocketStream>;

  SocketStream( Socket::SPtr sock, bool owner = true );
  ~SocketStream();

  virtual int read( void* buffer, size_t length ) override;
  virtual int read( ByteArray::SPtr ba, size_t length ) override;
  virtual int write( const void* buffer, size_t length ) override;
  virtual int write( ByteArray::SPtr ba, size_t length ) override;
  virtual void close() override;

  Socket::SPtr getSocket() const { return m_socket; }
  bool isConnected() const;

protected:
  Socket::SPtr m_socket;
  bool m_owner;
};

}