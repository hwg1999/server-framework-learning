#include "sylar/log.h"

int main()
{
  sylar::Logger::SPtr logger { std::make_shared<sylar::Logger>() };

  sylar::FileLogAppender::SPtr fileAppender { std::make_shared<sylar::FileLogAppender>( "./log.txt" ) };
  sylar::LogFormatter::SPtr formatter { std::make_shared<sylar::LogFormatter>( "%d%T%p%T%m%n" ) };
  fileAppender->setFormatter( formatter );
  fileAppender->setLevel( sylar::LogLevel::ERROR );
  logger->addAppender( fileAppender );
  logger->addAppender( std::make_shared<sylar::StdoutLogAppender>() );

  SYLAR_LOG_INFO( logger ) << "test macro";
  SYLAR_LOG_INFO( logger ) << "test macro error";
  SYLAR_LOG_FMT_ERROR( logger, "test macro fmt error %s", "aa" );

  auto xxLogger { sylar::LoggerMgr::GetInstance().getLogger( "xx" ) };
  SYLAR_LOG_INFO( xxLogger ) << "xxx";

  return 0;
}