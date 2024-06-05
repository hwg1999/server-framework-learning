/**
 * @file log.h
 * @brief 日志模块
 * @version 0.1
 * @date 2024-06-03
 *
 */
#pragma once

#include "sylar/singleton.h"
#include "sylar/thread.h"
#include "sylar/util.h"
#include <ctime>
#include <fstream>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

namespace sylar {

/**
 * @brief 使用流式方式将日志级别level的日志写入到logger
 * @details LogEventWrap对象包含日志器和日志事件，析构时调用logger写日志事件
 */
#define SYLAR_LOG_LEVEL( logger, level )                                                                           \
  if ( logger->getLevel() <= level )                                                                               \
  sylar::LogEventWrap( std::make_shared<sylar::LogEvent>( logger,                                                  \
                                                          level,                                                   \
                                                          __FILE__,                                                \
                                                          __LINE__,                                                \
                                                          sylar::GetElapsedMS() - logger->getCreateTime(),         \
                                                          sylar::GetThreadId(),                                    \
                                                          sylar::GetFiberId(),                                     \
                                                          std::time( 0 ),                                          \
                                                          sylar::Thread::GetName() ) )                             \
    .getSS()

/**
 * @brief 使用流式方式将日志级别为DEBUG的日志写入到logger
 */
#define SYLAR_LOG_DEBUG( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::DEBUG )

/**
 * @brief 使用流式方式将日志级别为INFO的日志写入到logger
 */
#define SYLAR_LOG_INFO( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::INFO )

/**
 * @brief 使用流式方式将日志级别为WARN的日志写入到logger
 */
#define SYLAR_LOG_WARN( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::WARN )

/**
 * @brief 使用流式方式将日志级别为ERROR的日志写入到logger
 */
#define SYLAR_LOG_ERROR( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::ERROR )

/**
 * @brief 使用流式方式将日志级别为FATAL的日志写入到logger
 */
#define SYLAR_LOG_FATAL( logger ) SYLAR_LOG_LEVEL( logger, sylar::LogLevel::FATAL )

/**
 * @brief 使用C printf风格的方式将日志级别为level的日志写入到logger
 * @details LogEventWrap对象包含日志器和日志事件，析构时调用logger写日志事件
 */
#define SYLAR_LOG_FMT_LEVEL( logger, level, fmt, ... )                                                             \
  if ( logger->getLevel() <= level )                                                                               \
  sylar::LogEventWrap( std::make_shared<sylar::LogEvent>( logger,                                                  \
                                                          level,                                                   \
                                                          __FILE__,                                                \
                                                          __LINE__,                                                \
                                                          0,                                                       \
                                                          sylar::GetThreadId(),                                    \
                                                          sylar::GetFiberId(),                                     \
                                                          std::time( 0 ),                                          \
                                                          sylar::Thread::GetName() ) )                             \
    .getEvent()                                                                                                    \
    ->format( fmt, __VA_ARGS__ )

/**
 * @brief 使用C printf风格的方式将日志级别为DEBUG的日志写入到logger
 */
#define SYLAR_LOG_FMT_DEBUG( logger, fmt, ... )                                                                    \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::DEBUG, fmt, __VA_ARGS__ )

/**
 * @brief 使用C printf风格的方式将日志级别为INFO的日志写入到logger
 */
#define SYLAR_LOG_FMT_INFO( logger, fmt, ... )                                                                     \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::INFO, fmt, __VA_ARGS__ )

/**
 * @brief 使用C printf风格的方式将日志级别为WARN的日志写入到logger
 */
#define SYLAR_LOG_FMT_WARN( logger, fmt, ... )                                                                     \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::WARN, fmt, __VA_ARGS__ )

/**
 * @brief 使用C printf风格的方式将日志级别为ERROR的日志写入到logger
 */
#define SYLAR_LOG_FMT_ERROR( logger, fmt, ... )                                                                    \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::ERROR, fmt, __VA_ARGS__ )

/**
 * @brief 使用C printf风格的方式将日志级别为FATAL的日志写入到logger
 */
#define SYLAR_LOG_FMT_FATAL( logger, fmt, ... )                                                                    \
  SYLAR_LOG_FMT_LEVEL( logger, sylar::LogLevel::FATAL, fmt, __VA_ARGS__ )

/**
 * @brief 获取root日志器，root日志器默认名称为空
 */
#define SYLAR_LOG_ROOT() sylar::LoggerMgr::GetInstance().getRoot()

/**
 * @brief 获取名称为name的日志器
 */
#define SYLAR_LOG_NAME( name ) sylar::LoggerMgr::GetInstance().getLogger( name )

/**
 * @brief 日志级别
 */
class LogLevel
{
public:
  /**
   * @brief 日志级别枚举
   */
  enum Level
  {
    UNKNOW = 0, // 未知级别
    DEBUG = 1,  // 调试信息
    INFO = 2,   // 一般信息
    WARN = 3,   // 警告信息
    ERROR = 4,  // 错误信息
    FATAL = 5   // 严重错误信息
  };

  /**
   * @brief 将日志级别转换为字符串
   * @param[in] level
   * @return 日志级别的字符串
   */
  static const char* ToString( LogLevel::Level level );

  /**
   * @brief 将文本转换为日志级别
   * @param[in] 日志级别的文本字符串
   * @return 日志级别
   * @note 不区分大小写
   */
  static LogLevel::Level FromString( std::string_view levelStr );
};

class Logger;
class LoggerManager;

/**
 * @brief 日志事件
 */
class LogEvent
{
public:
  using SPtr = std::shared_ptr<LogEvent>;

  /**
   * @brief 构造函数
   * @param[in] logger 日志器
   * @param[in] level 日志级别
   * @param[in] file 文件名
   * @param[in] line 行号
   * @param[in] elapse 从日志器创建开始到当前时刻的累计运行毫秒
   * @param[in] thread_id 线程id
   * @param[in] fiber_id 协程id
   * @param[in] time UTC时间
   * @param[in] thread_name 线程名称
   */
  LogEvent( std::shared_ptr<Logger> logger,
            LogLevel::Level level,
            const char* file,
            std::int32_t line,
            std::uint32_t elapse,
            std::uint32_t threadId,
            uint32_t fiberId,
            std::uint64_t time,
            std::string_view threadName );
  /**
   * @brief 获取日志级别
   */
  const char* getFile() const { return m_file; }

  /**
   * @brief 获取行号
   */
  std::int32_t getLine() const { return m_line; }

  /**
   * @brief 获取累计运行的毫秒数
   */
  std::uint32_t getElapse() const { return m_elapse; }

  /**
   * @brief 获取线程id
   */
  std::uint32_t getThreadId() const { return m_threadId; }

  /**
   * @brief 获取协程id
   */
  std::uint32_t getFiberId() const { return m_fiberId; }

  /**
   * @brief 获取时间戳
   */
  std::uint64_t getTime() const { return m_time; }

  /**
   * @brief 获取线程名称
   */
  std::string_view getThreadName() const { return m_threadName; }

  /**
   * @brief 获取日志内容
   */
  std::string getContent() const { return m_ss.str(); }

  /**
   * @brief 获取内容字节流，用于流式写入日志
   */
  std::stringstream& getSS() { return m_ss; }

  /**
   * @brief 获取日志器
   */
  std::shared_ptr<Logger> getLogger() const { return m_logger; }

  /**
   * @brief 获取日志级别
   */
  LogLevel::Level getLevel() const { return m_level; }

  /**
   * @brief C printf风格写入日志
   */
  void format( const char* fmt, ... );

  /**
   * @brief C vprintf风格写入日志
   */
  void format( const char* fmt, va_list al );

private:
  const char* m_file { nullptr };   // 文件名
  std::int32_t m_line { 0 };        // 行号
  std::uint32_t m_elapse { 0 };     // 程序启动开始到现在的毫秒数
  std::uint32_t m_threadId { 0 };   // 线程id
  std::uint32_t m_fiberId { 0 };    // 协程id
  std::uint64_t m_time { 0 };       // 时间戳
  std::string m_threadName;         // 线程名称
  std::stringstream m_ss;           // 输入内容的字符串流
  std::shared_ptr<Logger> m_logger; // 日志器
  LogLevel::Level m_level;          // 日志级别
};

/**
 * @brief 日志事件包装器
 */
class LogEventWrap
{
public:
  /**
   * @brief 构造函数
   * @param[in] event 日志事件
   */
  LogEventWrap( LogEvent::SPtr event );

  /**
   * @brief 日志事件在析构时由日志器输出
   */
  ~LogEventWrap();

  /**
   * @brief 日志事件
   */
  LogEvent::SPtr getEvent() const { return m_event; }

  /**
   * @brief 获取日志内容流
   */
  std::stringstream& getSS() const { return m_event->getSS(); }

private:
  LogEvent::SPtr m_event; // 日志事件
};

// 日志格式器
class LogFormatter
{
public:
  using SPtr = std::shared_ptr<LogFormatter>;

  /**
   * @brief 构造函数
   * @param[in] pattern 格式模板
   * @details
   *  %m 日志消息
   *  %p 日志级别
   *  %c 日志器名称
   *  %d 时间，可跟大括号指定时间的格式, 比如%d{%Y-%m-%d %H:%M:%S}, 与strftime的格式字符相同
   *  %r 累计毫秒数
   *  %f 文件名
   *  %l 行号
   *  %t 线程id
   *  %T 制表符
   *  %F 协程id
   *  %N 线程名称
   *  %n 换行
   *
   * 默认格式 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
   * 默认格式描述：年-月-日 时:分:秒 \t 线程id \t 线程名称 \t 协程id \t [日志级别] \t
   * [日志器名称] \t 文件名:行号 \t 日志消息 换行符
   */
  LogFormatter( std::string_view pattern );

  /**
   * @brief 格式化日志消息，返回格式化日志文本
   * @param[in] logger 日志器
   * @param[in] level  日志级别
   * @param[in] event  日志事件
   */
  std::string format( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event );

public:
  enum FormatStatus
  {
    REGULAR_SYMBOL = 0, // 普通符号
    INNER_SYMBOL,       // 被特殊解析的转义符号
  };

  /**
   * @brief 日志内容格式化项，接口描述，用于派生出不同的格式化项
   *
   */
  class FormatItem
  {
  public:
    using SPtr = std::shared_ptr<FormatItem>;

    /**
     * @brief 析构函数
     */
    virtual ~FormatItem() {}

    /**
     * @brief 格式化日志事件
     * @param[in] os 日志输出流
     * @param[in] logger 日志器
     * @param[in] level 日志等级
     * @param[in] event 日志事件
     */
    virtual void format( std::ostream& os,
                         std::shared_ptr<Logger> logger,
                         LogLevel::Level level,
                         LogEvent::SPtr event )
      = 0;
  };

  /**
   * @brief 解析日志模板
   */
  void init();

  /**
   * @brief 是否有错误
   */
  bool isError() const { return m_error; }

  /**
   * @brief 返回日志模板
   */
  std::string getPattern() const { return m_pattern; }

private:
  std::string m_pattern;                 // 日志格式模板
  std::vector<FormatItem::SPtr> m_items; // 解析后的格式模板数组
  bool m_error { false };                // 是否错误
};

/**
 * @brief 日志输出地
 */
class LogAppender
{
  friend class Logger;

public:
  using SPtr = std::shared_ptr<LogAppender>;
  using MutexType = SpinLock;

  /**
   * @brief 析构函数
   */
  virtual ~LogAppender() {}

  /**
   * @brief 写入日志
   * @param[in] logger 日志器
   * @param[in] level  日志级别
   * @param[in] event  日志事件
   */
  virtual void log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event ) = 0;

  /**
   * @brief 将日志输出目标的配置转为YAML String
   */
  virtual std::string toYamlString() = 0;

  /**
   * @brief 替换日志格式化器
   */
  void setFormatter( LogFormatter::SPtr val );

  /**
   * @brief 获取日志格式化器
   */
  LogFormatter::SPtr getFormatter();

  /**
   * @brief 获取日志级别
   */
  LogLevel::Level getLevel() const { return m_level; }

  /**
   * @brief 设置日志级别
   */
  void setLevel( LogLevel::Level val ) { m_level = val; }

protected:
  LogLevel::Level m_level { LogLevel::DEBUG }; // 日志级别
  bool m_hasFormatter { false };               // 是否有格式化器
  MutexType m_mutex;                           // Mutex
  LogFormatter::SPtr m_formatter;              // 日志格式化器
};

/**
 * @brief 日志器
 * @note 日志器类不带root logger
 */
class Logger : public std::enable_shared_from_this<Logger>
{
  friend class LoggerManager;

public:
  using SPtr = std::shared_ptr<Logger>;
  using MutexType = SpinLock;

  /**
   * @brief 构造函数
   * @param[in] name 日志器名称
   */
  Logger( std::string_view name = "root" );

  /**
   * @brief 写日志
   * @param[in] level 日志级别
   * @param[in] event 日志事件
   */
  void log( LogLevel::Level level, LogEvent::SPtr event );

  /**
   * @brief 写DEBUG级别日志
   * @param[in] event
   */
  void debug( LogEvent::SPtr event );

  /**
   * @brief 写INFO级别日志
   * @param[in] event
   */
  void info( LogEvent::SPtr event );

  /**
   * @brief 写WARN级别日志
   * @param[in] event
   */
  void warn( LogEvent::SPtr event );

  /**
   * @brief 写ERROR级别日志
   * @param[in] event
   */
  void error( LogEvent::SPtr event );

  /**
   * @brief 写FATAL级别日志
   * @param[in] event
   */
  void fatal( LogEvent::SPtr event );

  /**
   * @brief 添加LogAppender
   */
  void addAppender( LogAppender::SPtr appender );

  /**
   * @brief 删除LogAppender
   */
  void delAppender( LogAppender::SPtr appender );

  /**
   * @brief 清空LogAppender
   */
  void clearAppenders();

  /**
   * @brief 获取日志级别
   */
  LogLevel::Level getLevel() const { return m_level; }

  /**
   * @brief 设置日志级别
   */
  void setLevel( LogLevel::Level val ) { m_level = val; }

  /**
   * @brief 获取日志器名称
   */
  std::string_view getName() const { return m_name; }

  /**
   * @brief 设置日志格式器
   */
  void setFormatter( LogFormatter::SPtr val );

  /**
   * @brief 设置日志格式模板
   */
  void setFormatter( std::string_view val );

  /**
   * @brief 获取日志格式器
   */
  LogFormatter::SPtr getFormatter();

  /**
   * @brief 获取日志器创建的时间
   */
  uint64_t getCreateTime() { return m_createTime; }

  /**
   * @brief 将日志器的配置转成YAML String
   */
  std::string toYamlString();

private:
  std::string m_name;                       // 日志名称
  LogLevel::Level m_level;                  // 日志器级别
  MutexType m_mutex;                        // Mutex
  std::list<LogAppender::SPtr> m_appenders; // LogAppender几何
  LogFormatter::SPtr m_formatter;           // 日志格式器
  Logger::SPtr m_root;                      // 主日志器
  uint64_t m_createTime;                    /// 创建时间（毫秒）
};

/**
 * @brief 输出到控制台的LogAppender
 */
class StdoutLogAppender : public LogAppender
{
public:
  using SPtr = std::shared_ptr<StdoutLogAppender>;

  /**
   * @brief 写入日志
   */
  virtual void log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event ) override;

  /**
   * @brief 将日志输出目标的配置转成YAML String
   */
  virtual std::string toYamlString() override;
};

/**
 * @brief 输出到文件的LogAppender
 */
class FileLogAppender : public LogAppender
{
public:
  using SPtr = std::shared_ptr<FileLogAppender>;

  /**
   * @brief 构造函数
   * @param[in] filename 日志文件路径
   */
  FileLogAppender( std::string_view filename );

  /**
   * @brief 写日志
   */
  virtual void log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event ) override;

  /**
   * @brief 将日志输出目标的配置转成YAML String
   */
  virtual std::string toYamlString() override;

  /**
   * @brief 重新打开日志文件
   */
  bool reopen();

private:
  std::string m_filename;         // 文件路径
  std::ofstream m_filestream;     // 文件流
  std::uint64_t m_lastTime { 0 }; // 上次重新打开时间
};

/**
 * @brief 日志器管理类
 */
class LoggerManager
{
public:
  using MutexType = SpinLock;

  /**
   * @brief 构造函数
   */
  LoggerManager();

  /**
   * @brief 获取日志器
   * @param[in] name
   */
  Logger::SPtr getLogger( const std::string& name );

  /**
   * @brief 返回主日志器
   */
  Logger::SPtr getRoot() const { return m_root; }

  /**
   * @brief 初始化，结合配置模块实现日志模块初始化
   */
  void init();

  /**
   * @brief 将所有的日志器配置转成YAML String
   */
  std::string toYamlString();

private:
  MutexType m_mutex;                             // Mutex
  std::map<std::string, Logger::SPtr> m_loggers; // 日志器容器
  Logger::SPtr m_root;                           // 主日志器
};

// 日志器管理类单例
using LoggerMgr = sylar::Singleton<LoggerManager>;

} // namespace sylar
