#pragma once

#include "singleton.h"
#include "sylar/util.h"
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
    std::make_shared<sylar::LogEvent>(                                                                             \
      logger, level, __FILE__, __LINE__, 0, sylar::GetThreadId(), sylar::GetFiberId(), std::time( 0 ) ) )          \
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

#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance().getRoot()
#define SYLAR_LOG_NAME( name ) sylar::LoggerMgr::GetInstance().getLogger( name )

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
  static LogLevel::Level FromString( const std::string& str );
};

class Logger;
class LoggerManager;

// 日志事件类
class LogEvent
{
public:
  using SPtr = std::shared_ptr<LogEvent>;

  LogEvent( std::shared_ptr<Logger> logger,
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
  std::shared_ptr<Logger> getLogger() const { return m_logger; }
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
  std::shared_ptr<Logger> m_logger;
  LogLevel::Level m_level;
};

class LogEventWrap
{
public:
  LogEventWrap( LogEvent::SPtr event );
  ~LogEventWrap();
  LogEvent::SPtr getEvent() const { return m_event; }
  std::stringstream& getSS() const { return m_event->getSS(); }

private:
  LogEvent::SPtr m_event;
};

// 日志格式器
class LogFormatter
{
public:
  using SPtr = std::shared_ptr<LogFormatter>;

  LogFormatter( const std::string& pattern );
  std::string format( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event );

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

  class FormatItem
  {
  public:
    using SPtr = std::shared_ptr<FormatItem>;

    virtual ~FormatItem() {}
    virtual void format( std::ostream& os,
                         std::shared_ptr<Logger> logger,
                         LogLevel::Level level,
                         LogEvent::SPtr event )
      = 0;
  };

  void init();

  bool isError() const { return m_error; }
  std::string getPattern() const { return m_pattern; }

private:
  std::string m_pattern;
  std::vector<FormatItem::SPtr> m_items;
  bool m_error { false };
};

// 日志输出地
class LogAppender
{
public:
  using SPtr = std::shared_ptr<LogAppender>;

  virtual ~LogAppender() {}
  virtual void log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event ) = 0;
  virtual std::string toYamlString() = 0;

  void setFormatter( LogFormatter::SPtr val ) { m_formatter = val; }
  LogFormatter::SPtr getFormatter() const { return m_formatter; }

  LogLevel::Level getLevel() const { return m_level; }
  void setLevel( LogLevel::Level val ) { m_level = val; }

protected:
  LogLevel::Level m_level;
  LogFormatter::SPtr m_formatter;
};

// 日志器
class Logger : public std::enable_shared_from_this<Logger>
{
  friend class LoggerManager;

public:
  using SPtr = std::shared_ptr<Logger>;

  Logger( const std::string& name = "root" );
  void log( LogLevel::Level level, LogEvent::SPtr event );

  void debug( LogEvent::SPtr event );
  void info( LogEvent::SPtr event );
  void warn( LogEvent::SPtr event );
  void error( LogEvent::SPtr event );
  void fatal( LogEvent::SPtr event );

  void addAppender( LogAppender::SPtr appender );
  void delAppender( LogAppender::SPtr appender );
  void clearAppenders();
  LogLevel::Level getLevel() const { return m_level; }
  void setLevel( LogLevel::Level val ) { m_level = val; }

  const std::string& getName() const { return m_name; }

  void setFormatter( LogFormatter::SPtr val );
  void setFormatter( const std::string& val );
  LogFormatter::SPtr getFormatter();

  std::string toYamlString();

private:
  std::string m_name;                       // 日志名称
  LogLevel::Level m_level;                  // 满足日志级别才能输出
  std::list<LogAppender::SPtr> m_appenders; // Appender列表
  LogFormatter::SPtr m_formatter;
  Logger::SPtr m_root;
};

// 输出到控制台的Appender
class StdoutLogAppender : public LogAppender
{
public:
  using SPtr = std::shared_ptr<StdoutLogAppender>;
  virtual void log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event ) override;
  virtual std::string toYamlString() override;
};

// 输出到文件的Appender
class FileLogAppender : public LogAppender
{
public:
  using SPtr = std::shared_ptr<FileLogAppender>;

  FileLogAppender( const std::string& filename );
  virtual void log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event ) override;
  virtual std::string toYamlString() override;

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
  Logger::SPtr getLogger( const std::string& name );

  Logger::SPtr getRoot() const { return m_root; }
  void init();

  std::string toYamlString();
private:
  std::map<std::string, Logger::SPtr> m_loggers;
  Logger::SPtr m_root;
};

using LoggerMgr = sylar::Singleton<LoggerManager>;

} // namespace sylar
