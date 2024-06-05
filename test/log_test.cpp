#include "sylar/log.h"
#include <memory>

int main()
{
  sylar::Logger::SPtr logger { std::make_shared<sylar::Logger>() };

  sylar::FileLogAppender::SPtr fileAppender { std::make_shared<sylar::FileLogAppender>( "./log.txt" ) };
  sylar::LogFormatter::SPtr formatter { std::make_shared<sylar::LogFormatter>( "%d%T%p%T%m%n" ) };
  fileAppender->setFormatter( formatter );
  fileAppender->setLevel( sylar::LogLevel::ERROR );
  logger->addAppender( fileAppender );
  logger->addAppender( std::make_shared<sylar::StdoutLogAppender>() );

  SYLAR_LOG_DEBUG( logger ) << "debug msg";
  SYLAR_LOG_INFO( logger ) << "info msg";
  SYLAR_LOG_WARN( logger ) << "warn msg";
  SYLAR_LOG_ERROR( logger ) << "error msg";
  SYLAR_LOG_FATAL( logger ) << "fatal msg";

  SYLAR_LOG_FMT_DEBUG( logger, "debug %s:%d", __FILE__, __LINE__ );
  SYLAR_LOG_FMT_INFO( logger, "info %s:%d", __FILE__, __LINE__ );
  SYLAR_LOG_FMT_WARN( logger, "warn %s:%d", __FILE__, __LINE__ );
  SYLAR_LOG_FMT_ERROR( logger, "error %s:%d", __FILE__, __LINE__ );
  SYLAR_LOG_FMT_FATAL( logger, "fatal %s:%d", __FILE__, __LINE__ );

  logger->setLevel( sylar::LogLevel::WARN );
  SYLAR_LOG_DEBUG( logger ) << "debug msg"; // 不打印
  SYLAR_LOG_INFO( logger ) << "info msg";   // 不打印
  SYLAR_LOG_WARN( logger ) << "warn msg";
  SYLAR_LOG_ERROR( logger ) << "error msg";
  SYLAR_LOG_FATAL( logger ) << "fatal msg";

  sylar::Logger::SPtr testLogger = SYLAR_LOG_NAME( "test_logger" );
  sylar::StdoutLogAppender::SPtr appender { std::make_shared<sylar::StdoutLogAppender>() };
  sylar::LogFormatter::SPtr testFormatter { std::make_shared<sylar::LogFormatter>(
    "%d:%rms%T%p%T%c%T%f:%l %m%n" ) }; // 时间：启动毫秒数 级别 日志名称 文件名：行号 消息 换行
  appender->setFormatter( testFormatter );
  testLogger->addAppender( appender );
  testLogger->setLevel( sylar::LogLevel::WARN );

  SYLAR_LOG_INFO( testLogger ) << "info msg";
  SYLAR_LOG_ERROR( testLogger ) << "error msg";

  auto xxLogger { sylar::LoggerMgr::GetInstance().getLogger( "xx" ) };
  SYLAR_LOG_INFO( xxLogger ) << "xxx";

  // 输出全部日志器的配置
  logger->setLevel( sylar::LogLevel::INFO );
  SYLAR_LOG_INFO( logger ) << "logger config: " << sylar::LoggerMgr::GetInstance().toYamlString();

  return 0;
}