#pragma once

#include "singleton.h"
#include "util.h"
#include <cstdarg>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace sylar {

#define SYLAR_LOG_LEVEL( logger, level )                                                                           \
  if ( logger->getLevel() <= level )                                                                               \
  sylar::LogEventWrap(                                                                                             \
    std::make_shared<sylar::LogEvent>(                                                                             \
      logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), std::time( 0 ) ) )          \
    .getSS()

#define SYLAR_LOG_DEBUG( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::DEBUG )
#define SYLAR_LOG_INFO( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::INFO )
#define SYLAR_LOG_WARN( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::WARN )
#define SYLAR_LOG_ERROR( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::ERROR )
#define SYLAR_LOG_FATAL( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::FATAL )

#define SYLAR_LOG_FMT_LEVEL( logger, level, fmt, ... )                                                             \
  if ( logger->getLevel() <= level )                                                                               \
  sylar::LogEventWrap(                                                                                             \
    sylar::LogEventSPtr( std::make_shared<sylar::LogEvent>(                                                        \
      logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), std::time( 0 ) ) ) )        \
    .getEvent()                                                                                                    \
    ->format( fmt, __VA_ARGS__ )

#define SYLAR_LOG_FMT_DEBUG( logger, fmt, ... )                                                                    \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__ )
#define SYLAR_LOG_FMT_INFO( logger, fmt, ... )                                                                     \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__ )
#define SYLAR_LOG_FMT_WARN( logger, fmt, ... )                                                                     \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__ )
#define SYLAR_LOG_FMT_ERROR( logger, fmt, ... )                                                                    \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__ )
#define SYLAR_LOG_FMT_FATAL( logger, fmt, ... )                                                                    \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__ )

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance()->getRoot()

#define SHARED_PTR_DECLEARE( className )                                                                           \
  class className;                                                                                                 \
  using className##SPtr = std::shared_ptr<className>;

SHARED_PTR_DECLEARE( LogEvent )
SHARED_PTR_DECLEARE( LogFormatter )
SHARED_PTR_DECLEARE( LogAppender )
SHARED_PTR_DECLEARE( Logger )
SHARED_PTR_DECLEARE( StdoutLogAppender )
SHARED_PTR_DECLEARE( FileLogAppender )

// 日志级别
class LogLevel
{
public:
  enum Level
  {
    UNKNOW = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
  };

  static const char* ToString( LogLevel::Level level );
};

// 日志事件类
SHARED_PTR_DECLEARE( LogEvent )
class LogEvent
{
public:
  LogEvent( LoggerSPtr logger,
            LogLevel::Level level,
            const char* file,
            std::int32_t line,
            std::uint32_t elapse,
            std::uint32_t thread_id,
            uint32_t fiber_id,
            std::uint64_t time );

  const char* getFile() const { return m_file; }
  std::int32_t getLine() const { return m_line; }
  std::uint32_t getElapse() const { return m_elapse; }
  std::uint32_t getThreadId() const { return m_threadId; }
  std::uint32_t getFiberId() const { return m_fiberId; }
  std::uint64_t getTime() const { return m_time; }
  std::string getContent() const { return m_ss.str(); }
  std::stringstream& getSS() { return m_ss; }
  LoggerSPtr getLogger() const { return m_logger; }
  LogLevel::Level getLevel() const { return m_level; }

  void format( const char* fmt, ... );
  void format( const char* fmt, va_list al );

private:
  const char* m_file { nullptr }; // 文件名
  std::int32_t m_line { 0 };      // 行号
  std::uint32_t m_elapse { 0 };   // 程序启动开始到现在的毫秒数
  std::uint32_t m_threadId { 0 }; // 线程id
  std::uint32_t m_fiberId { 0 };  // 协程id
  std::uint64_t m_time { 0 };     // 时间戳
  std::stringstream m_ss;
  LoggerSPtr m_logger;
  LogLevel::Level m_level;
};

class LogEventWrap
{
public:
  LogEventWrap( LogEventSPtr event );
  ~LogEventWrap();
  LogEventSPtr getEvent() const { return m_event; }
  std::stringstream& getSS() const { return m_event->getSS(); }

private:
  LogEventSPtr m_event;
};

// 日志格式器
class LogFormatter
{
public:
  LogFormatter( const std::string& pattern );
  std::string format( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEventSPtr event );

public:
  enum ParseResultIdx
  {
    RAW_STRING = 0,
    FORMAT_STRING,
    FORMAT_STATUS,
  };

  enum FormatStatus
  {
    REGULAR_SYMBOL,
    INNER_SYMBOL,
  };

  SHARED_PTR_DECLEARE( FormatItem );
  class FormatItem
  {
  public:
    virtual ~FormatItem() {}
    virtual void format( std::ostream& os,
                         std::shared_ptr<Logger> logger,
                         LogLevel::Level level,
                         LogEventSPtr event )
      = 0;
  };

  void init();

private:
  std::string m_pattern;
  std::vector<FormatItemSPtr> m_items;
};

// 日志输出地
class LogAppender
{
public:
  virtual ~LogAppender() {}
  virtual void log( LoggerSPtr logger, LogLevel::Level level, LogEventSPtr event ) = 0;

  void setFormatter( LogFormatterSPtr val ) { m_formatter = val; }
  LogFormatterSPtr getFormatter() const { return m_formatter; }

  LogLevel::Level getLevel() const { return m_level; }
  void setLevel( LogLevel::Level val ) { m_level = val; }

protected:
  LogLevel::Level m_level;
  LogFormatterSPtr m_formatter;
};

// 日志器
class Logger : public std::enable_shared_from_this<Logger>
{
public:
  Logger( const std::string& name = "root" );
  void log( LogLevel::Level level, LogEventSPtr event );

  void debug( LogEventSPtr event );
  void info( LogEventSPtr event );
  void warn( LogEventSPtr event );
  void error( LogEventSPtr event );
  void fatal( LogEventSPtr event );

  void addAppender( LogAppenderSPtr appender );
  void delAppender( LogAppenderSPtr appender );
  LogLevel::Level getLevel() const { return m_level; }
  void setLevel( LogLevel::Level val ) { m_level = val; }

  const std::string& getName() const { return m_name; }

private:
  std::string m_name;                     // 日志名称
  LogLevel::Level m_level;                // 满足日志级别才能输出
  std::list<LogAppenderSPtr> m_appenders; // Appender列表
  LogFormatterSPtr m_formatter;
};

// 输出到控制台的Appender
class StdoutLogAppender : public LogAppender
{
public:
  virtual void log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEventSPtr event ) override;
};

// 输出到文件的Appender
class FileLogAppender : public LogAppender
{
public:
  FileLogAppender( const std::string& filename );
  void log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEventSPtr event ) override;

  // 重新打开文件返回true
  bool reopen();

private:
  std::string m_filename;
  std::ofstream m_filestream;
};

class LoggerManager
{
public:
  LoggerManager();
  LoggerSPtr getLogger( const std::string& name );
  LoggerSPtr getRoot() const { return m_root; }

private:
  std::map<std::string, LoggerSPtr> m_loggers;
  LoggerSPtr m_root;
};

using LoggerMgr = sylar::Singleton<LoggerManager>;

} // namespace sylar
