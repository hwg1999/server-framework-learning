#pragma once

#include "sylar/address.h"
#include "sylar/iomanager.h"
#include "sylar/noncopyable.h"
#include "sylar/socket.h"
#include <memory>
#include <vector>

namespace sylar {

class TcpServer
  : public std::enable_shared_from_this<TcpServer>
  , Noncopyable
{
public:
  using SPtr = std::shared_ptr<TcpServer>;

  TcpServer( sylar::IOManager* worker = sylar::IOManager::GetThis(),
             sylar::IOManager* accept_worker = sylar::IOManager::GetThis() );
  virtual ~TcpServer();

  virtual bool bind( sylar::Address::SPtr addr );
  virtual bool bind( const std::vector<Address::SPtr>& addrs, std::vector<Address::SPtr>& fails );
  virtual bool start();
  virtual void stop();

  uint64_t getRecvTimeout() const { return m_recvTimeout; }
  std::string getName() const { return m_name; }
  void setRecvTimeout( uint64_t val ) { m_recvTimeout = val; }
  void setName( const std::string& val ) { m_name = val; }

  bool isStop() const { return m_isStop; }

protected:
  virtual void handleClient( Socket::SPtr client );
  virtual void startAccept( Socket::SPtr sock );

private:
  std::vector<Socket::SPtr> m_socks;
  IOManager* m_worker;
  IOManager* m_acceptWorker;
  uint64_t m_recvTimeout;
  std::string m_name;
  bool m_isStop;
};

}