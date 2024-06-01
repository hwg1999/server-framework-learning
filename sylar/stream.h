#pragma once

#include "sylar/bytearray.h"
#include <memory>

namespace sylar {

class Stream
{
public:
  using SPtr = std::shared_ptr<Stream>;

  virtual ~Stream() {}
  virtual int read( void* buffer, size_t length ) = 0;
  virtual int read( ByteArray::SPtr ba, size_t length ) = 0;
  virtual int readFixSize( void* buffer, size_t length );
  virtual int readFixSize( ByteArray::SPtr ba, size_t length );
  virtual int write( const void* buffer, size_t length ) = 0;
  virtual int write( ByteArray::SPtr ba, size_t length ) = 0;
  virtual int writeFixSize( const void* buffer, size_t length );
  virtual int writeFixSize( ByteArray::SPtr ba, size_t length );
  virtual void close() = 0;
};

}