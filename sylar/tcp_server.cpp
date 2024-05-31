#include "sylar/tcp_server.h"
#include "sylar/log.h"
#include "sylar/address.h"
#include "sylar/config.h"
#include "sylar/iomanager.h"
#include "sylar/socket.h"
#include <cstring>
#include <functional>
#include <vector>

namespace sylar {

static sylar::ConfigVar<uint64_t>::SPtr g_tcp_server_read_timeout
  = sylar::Config::Lookup( "tcp_server.read_timeout", (uint64_t)( 60 * 1000 * 2 ), "tcp server read timeout" );

static sylar::Logger::SPtr g_logger = SYLAR_LOG_NAME( "system" );

TcpServer::TcpServer( sylar::IOManager* worker, sylar::IOManager* accept_worker )
  : m_worker { worker }
  , m_acceptWorker { accept_worker }
  , m_recvTimeout { g_tcp_server_read_timeout->getValue() }
  , m_name { "sylar/1.0.0" }
  , m_isStop { true }
{}

TcpServer::~TcpServer()
{
  for ( auto& sock : m_socks ) {
    sock->close();
  }

  m_socks.clear();
}

bool TcpServer::bind( sylar::Address::SPtr addr )
{
  std::vector<Address::SPtr> addrs;
  std::vector<Address::SPtr> fails;
  addrs.push_back( addr );
  return bind( addrs, fails );
}

bool TcpServer::bind( const std::vector<Address::SPtr>& addrs, std::vector<Address::SPtr>& fails )
{
  for ( const Address::SPtr& addr : addrs ) {
    Socket::SPtr sock { Socket::CreateTCP( addr ) };
    if ( !sock->bind( addr ) ) {
      SYLAR_LOG_ERROR( g_logger ) << "bind fail errno=" << errno << " errstr=" << strerror( errno ) << " addr=["
                                  << addr->toString() << "]";
      fails.push_back( addr );
      continue;
    }

    if ( !sock->listen() ) {
      SYLAR_LOG_ERROR( g_logger ) << "listen fail errno=" << errno << " errstr=" << strerror( errno ) << " addr=["
                                  << addr->toString() << "]";
      fails.push_back( addr );
      continue;
    }
    m_socks.push_back( sock );
  }

  if ( !fails.empty() ) {
    m_socks.clear();
    return false;
  }

  for ( Socket::SPtr& sock : m_socks ) {
    SYLAR_LOG_INFO( g_logger ) << "server bind success: " << *sock;
  }
  return true;
}

void TcpServer::startAccept( Socket::SPtr sock )
{
  while ( !m_isStop ) {
    Socket::SPtr client = sock->accept();
    if ( client ) {
      client->setRecvTimeout( m_recvTimeout );
      m_worker->schedule( std::bind( &TcpServer::handleClient, shared_from_this(), client ) );
    } else {
      SYLAR_LOG_ERROR( g_logger ) << "accept errno=" << errno << " errstr=" << strerror( errno );
    }
  }
}

bool TcpServer::start()
{
  if ( !m_isStop ) {
    return true;
  }
  m_isStop = false;
  for ( const Socket::SPtr& sock : m_socks ) {
    m_acceptWorker->schedule( std::bind( &TcpServer::startAccept, shared_from_this(), sock ) );
  }

  return true;
}

void TcpServer::stop()
{
  m_isStop = true;
  auto self = shared_from_this();
  m_acceptWorker->schedule( [self, this]() {
    for ( const Socket::SPtr& sock : m_socks ) {
      sock->cancelAll();
      sock->close();
    }
    m_socks.clear();
  } );
}

void TcpServer::handleClient( Socket::SPtr client )
{
  SYLAR_LOG_INFO( g_logger ) << "handleClient: " << *client;
}

}