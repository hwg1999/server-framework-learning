#include "sylar/iomanager.h"
#include "sylar/log.h"
#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

sylar::Logger::SPtr g_logger { SYLAR_LOG_ROOT() };

int sock { 0 };

void test_fiber()
{
  SYLAR_LOG_INFO( g_logger ) << "test_fiber sock = " << sock;

  sock = socket( AF_INET, SOCK_STREAM, 0 );
  fcntl( sock, F_SETFL, O_NONBLOCK );

  sockaddr_in addr;
  std::memset( &addr, 0, sizeof( addr ) );
  addr.sin_family = AF_INET;
  addr.sin_port = htons( 80 );
  inet_pton( AF_INET, "127.0.0.1", &addr.sin_addr.s_addr );

  int ret = connect( sock, (const sockaddr*)&addr, sizeof( addr ) );
  if ( !ret ) {
    if ( errno == EINPROGRESS ) {
      SYLAR_LOG_INFO( g_logger ) << "add event errno = " << errno << " " << strerror( errno );
      sylar::IOManager::GetThis()->addEvent(
        sock, sylar::IOManager::READ, []() { SYLAR_LOG_INFO( g_logger ) << "read callback"; } );
      sylar::IOManager::GetThis()->addEvent( sock, sylar::IOManager::WRITE, []() {
        SYLAR_LOG_INFO( g_logger ) << "write callback";
        sylar::IOManager::GetThis()->cancelEvent( sock, sylar::IOManager::READ );
        close( sock );
      } );
    } else {
      SYLAR_LOG_INFO( g_logger ) << "else" << errno << " " << strerror( errno );
    }
  }
}

void test1()
{
  std::cout << "EPOLLIN = " << EPOLLIN << " EPOLLOUT = " << EPOLLOUT << std::endl;
  sylar::IOManager iomanager { 2, false };
  iomanager.schedule( &test_fiber );
}

int main()
{
  test1();
  return 0;
}