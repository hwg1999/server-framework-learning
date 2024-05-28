#include "sylar/bytearray.h"
#include "sylar/log.h"
#include "sylar/macro.h"
#include <cassert>
#include <memory>
#include <vector>

static sylar::Logger::SPtr g_logger = SYLAR_LOG_ROOT();

void test()
{
#define XX( type, len, write_fun, read_fun, base_len )                                                             \
  {                                                                                                                \
    std::vector<type> vec;                                                                                         \
    for ( int i = 0; i < len; ++i ) {                                                                              \
      vec.push_back( rand() );                                                                                     \
    }                                                                                                              \
    sylar::ByteArray::SPtr ba = std::make_shared<sylar::ByteArray>( base_len );                                    \
    for ( auto& i : vec ) {                                                                                        \
      ba->write_fun( i );                                                                                          \
    }                                                                                                              \
    ba->setPosition( 0 );                                                                                          \
    for ( size_t i = 0; i < vec.size(); ++i ) {                                                                    \
      type v = ba->read_fun();                                                                                     \
      SYLAR_ASSERT( v == vec[i] );                                                                                 \
    }                                                                                                              \
    SYLAR_ASSERT( ba->getReadSize() == 0 );                                                                        \
    SYLAR_LOG_INFO( g_logger ) << #write_fun "/" #read_fun " (" #type " ) len=" << len << " base_len=" << base_len \
                               << " size=" << ba->getSize();                                                       \
  }

  XX( int8_t, 100, writeFint8, readFint8, 1 );
  XX( uint8_t, 100, writeFuint8, readFuint8, 1 );
  XX( int16_t, 100, writeFint16, readFint16, 1 );
  XX( uint16_t, 100, writeFuint16, readFuint16, 1 );
  XX( int32_t, 100, writeFint32, readFint32, 1 );
  XX( uint32_t, 100, writeFuint32, readFuint32, 1 );
  XX( int64_t, 100, writeFint64, readFint64, 1 );
  XX( uint64_t, 100, writeFuint64, readFuint64, 1 );

  XX( int32_t, 100, writeInt32, readInt32, 1 );
  XX( uint32_t, 100, writeUint32, readUint32, 1 );
  XX( int64_t, 100, writeInt64, readInt64, 1 );
  XX( uint64_t, 100, writeUint64, readUint64, 1 );
#undef XX

#define XX( type, len, write_fun, read_fun, base_len )                                                             \
  {                                                                                                                \
    std::vector<type> vec;                                                                                         \
    for ( int i = 0; i < len; ++i ) {                                                                              \
      vec.push_back( rand() );                                                                                     \
    }                                                                                                              \
    sylar::ByteArray::SPtr ba( new sylar::ByteArray( base_len ) );                                                 \
    for ( auto& i : vec ) {                                                                                        \
      ba->write_fun( i );                                                                                          \
    }                                                                                                              \
    ba->setPosition( 0 );                                                                                          \
    for ( size_t i = 0; i < vec.size(); ++i ) {                                                                    \
      type v = ba->read_fun();                                                                                     \
      SYLAR_ASSERT( v == vec[i] );                                                                                 \
    }                                                                                                              \
    SYLAR_ASSERT( ba->getReadSize() == 0 );                                                                        \
    SYLAR_LOG_INFO( g_logger ) << #write_fun "/" #read_fun " (" #type " ) len=" << len << " base_len=" << base_len \
                               << " size=" << ba->getSize();                                                       \
    ba->setPosition( 0 );                                                                                          \
    SYLAR_ASSERT( ba->writeToFile( "/tmp/" #type "_" #len "-" #read_fun ".dat" ) );                                \
    sylar::ByteArray::SPtr ba2( new sylar::ByteArray( base_len * 2 ) );                                            \
    SYLAR_ASSERT( ba2->readFromFile( "/tmp/" #type "_" #len "-" #read_fun ".dat" ) );                              \
    ba2->setPosition( 0 );                                                                                         \
    SYLAR_ASSERT( ba->toString() == ba2->toString() );                                                             \
    SYLAR_ASSERT( ba->getPosition() == 0 );                                                                        \
    SYLAR_ASSERT( ba2->getPosition() == 0 );                                                                       \
  }
  XX( int8_t, 100, writeFint8, readFint8, 1 );
  XX( uint8_t, 100, writeFuint8, readFuint8, 1 );
  XX( int16_t, 100, writeFint16, readFint16, 1 );
  XX( uint16_t, 100, writeFuint16, readFuint16, 1 );
  XX( int32_t, 100, writeFint32, readFint32, 1 );
  XX( uint32_t, 100, writeFuint32, readFuint32, 1 );
  XX( int64_t, 100, writeFint64, readFint64, 1 );
  XX( uint64_t, 100, writeFuint64, readFuint64, 1 );

  XX( int32_t, 100, writeInt32, readInt32, 1 );
  XX( uint32_t, 100, writeUint32, readUint32, 1 );
  XX( int64_t, 100, writeInt64, readInt64, 1 );
  XX( uint64_t, 100, writeUint64, readUint64, 1 );

#undef XX
}

int main()
{
  test();
  return 0;
}
