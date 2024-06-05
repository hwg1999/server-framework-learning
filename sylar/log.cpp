#include "sylar/log.h"
#include "sylar/config.h"
#include "sylar/util.h"
#include <cctype>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <ostream>
#include <set>
#include <string_view>
#include <utility>
#include <vector>
#include <yaml-cpp/node/node.h>

namespace sylar {

const char* LogLevel::ToString( LogLevel::Level level )
{
  switch ( level ) {
#define XX( name )                                                                                                 \
  case LogLevel::name:                                                                                             \
    return #name;
    XX( DEBUG );
    XX( INFO );
    XX( WARN );
    XX( ERROR );
    XX( FATAL );
#undef XX
    default:
      return "UNKNOW";
  }

  return "UNKNOW";
}

LogLevel::Level LogLevel::FromString( std::string_view levelStr )
{
#define XX( level, val )                                                                                           \
  if ( levelStr == #val ) {                                                                                        \
    return LogLevel::level;                                                                                        \
  }

  XX( DEBUG, debug );
  XX( INFO, info );
  XX( WARN, warn );
  XX( ERROR, error );
  XX( FATAL, fatal );

  XX( DEBUG, DEBUG );
  XX( INFO, INFO );
  XX( WARN, WARN );
  XX( ERROR, ERROR );
  XX( FATAL, FATAL );

#undef XX
  return LogLevel::UNKNOW;
}

LogEvent::LogEvent( Logger::SPtr logger,
                    LogLevel::Level level,
                    const char* file,
                    std::int32_t line,
                    std::uint32_t elapse,
                    std::uint32_t threadId,
                    std::uint32_t fiberId,
                    std::uint64_t time,
                    std::string_view threadName )
  : m_file { file }
  , m_line { line }
  , m_elapse { elapse }
  , m_threadId { threadId }
  , m_fiberId { fiberId }
  , m_time { time }
  , m_threadName { threadName }
  , m_logger { logger }
  , m_level { level }
{}

void LogEvent::format( const char* fmt, ... )
{
  va_list al;
  va_start( al, fmt );
  format( fmt, al );
  va_end( al );
}

void LogEvent::format( const char* fmt, va_list al )
{
  char* buf = nullptr;
  int len = vasprintf( &buf, fmt, al );
  if ( len != 1 ) {
    m_ss << std::string( buf, len );
    std::free( buf );
  }
}

LogEventWrap::LogEventWrap( LogEvent::SPtr event ) : m_event { event } {}

/**
 * @brief LogEventWrap析构时写日志
 *
 */
LogEventWrap::~LogEventWrap()
{
  m_event->getLogger()->log( m_event->getLevel(), m_event );
}

class MessageFormatItem : public LogFormatter::FormatItem
{
public:
  MessageFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os,
               std::shared_ptr<Logger> logger,
               LogLevel::Level level,
               LogEvent::SPtr event ) override
  {
    os << event->getContent();
  }
};

class LevelFormatItem : public LogFormatter::FormatItem
{
public:
  LevelFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os,
               std::shared_ptr<Logger> logger,
               LogLevel::Level level,
               LogEvent::SPtr event ) override
  {
    os << LogLevel::ToString( level );
  }
};

class ElapseFormatItem : public LogFormatter::FormatItem
{
public:
  ElapseFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os,
               std::shared_ptr<Logger> logger,
               LogLevel::Level level,
               LogEvent::SPtr event ) override
  {
    os << event->getElapse();
  }
};

class NameFormatItem : public LogFormatter::FormatItem
{
public:
  NameFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os, Logger::SPtr logger, LogLevel::Level level, LogEvent::SPtr event ) override
  {
    os << logger->getName();
  }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem
{
public:
  ThreadIdFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os,
               std::shared_ptr<Logger> logger,
               LogLevel::Level level,
               LogEvent::SPtr event ) override
  {
    os << event->getThreadId();
  }
};

class FiberIdFormatItem : public LogFormatter::FormatItem
{
public:
  FiberIdFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os,
               std::shared_ptr<Logger> logger,
               LogLevel::Level level,
               LogEvent::SPtr event ) override
  {
    os << event->getFiberId();
  }
};

class ThreadNameFormatItem : public LogFormatter::FormatItem
{
public:
  ThreadNameFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os, Logger::SPtr logger, LogLevel::Level level, LogEvent::SPtr event ) override
  {
    os << event->getThreadName();
  }
};

class DateTimeFormatItem : public LogFormatter::FormatItem
{
public:
  DateTimeFormatItem( std::string_view format = "%Y:%m:%d %H:%M:%S" ) : m_format { format }
  {
    if ( format.empty() ) {
      m_format = "%Y:%m:%d %H:%M:%S";
    }
  }

  void format( std::ostream& os,
               std::shared_ptr<Logger> logger,
               LogLevel::Level level,
               LogEvent::SPtr event ) override
  {
    struct tm tm;
    std::time_t time = event->getTime();
    localtime_r( &time, &tm );
    char buf[64];
    strftime( buf, sizeof( buf ), m_format.c_str(), &tm );
    os << buf;
  }

private:
  std::string m_format;
};

class FilenameFormatItem : public LogFormatter::FormatItem
{
public:
  FilenameFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os,
               std::shared_ptr<Logger> logger,
               LogLevel::Level level,
               LogEvent::SPtr event ) override
  {
    os << event->getFile();
  }
};

class LineFormatItem : public LogFormatter::FormatItem
{
public:
  LineFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os, Logger::SPtr logger, LogLevel::Level level, LogEvent::SPtr event ) override
  {
    os << event->getLine();
  }
};

class NewLineFormatItem : public LogFormatter::FormatItem
{
public:
  NewLineFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os,
               std::shared_ptr<Logger> logger,
               LogLevel::Level level,
               LogEvent::SPtr event ) override
  {
    os << std::endl;
  }
};

class StringFormatItem : public LogFormatter::FormatItem
{
public:
  StringFormatItem( std::string_view str ) : m_string { str } {}
  void format( std::ostream& os, Logger::SPtr logger, LogLevel::Level level, LogEvent::SPtr event ) override
  {
    os << m_string;
  }

private:
  std::string m_string;
};

class TabFormatItem : public LogFormatter::FormatItem
{
public:
  TabFormatItem( std::string_view str = "" ) {}
  void format( std::ostream& os,
               std::shared_ptr<Logger> logger,
               LogLevel::Level level,
               LogEvent::SPtr event ) override
  {
    os << '\t';
  }
};

void LogAppender::setFormatter( LogFormatter::SPtr val )
{
  MutexType::Lock lock { m_mutex };
  m_formatter = val;
  m_hasFormatter = m_formatter ? true : false;
}

LogFormatter::SPtr LogAppender::getFormatter()
{
  MutexType::Lock lock { m_mutex };
  return m_formatter;
}

Logger::Logger( std::string_view name )
  : m_name { name }, m_level { LogLevel::DEBUG }, m_createTime { GetElapsedMS() }
{
  m_formatter.reset( new LogFormatter( "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n" ) );
}

void Logger::addAppender( LogAppender::SPtr appender )
{
  MutexType::Lock lock( m_mutex );
  if ( !appender->getFormatter() ) {
    appender->setFormatter( m_formatter );
  }
  m_appenders.push_back( appender );
}

void Logger::delAppender( LogAppender::SPtr appender )
{
  MutexType::Lock lock( m_mutex );
  for ( auto it = m_appenders.begin(); it != m_appenders.end(); ++it ) {
    if ( *it == appender ) {
      m_appenders.erase( it );
      break;
    }
  }
}

void Logger::clearAppenders()
{
  MutexType::Lock lock( m_mutex );
  m_appenders.clear();
}

/**
 * @brief 调用Logger的所有appenders写一遍日志
 */
void Logger::log( LogLevel::Level level, LogEvent::SPtr event )
{
  if ( level >= m_level ) {
    auto self = shared_from_this();
    MutexType::Lock lock( m_mutex );
    if ( !m_appenders.empty() ) {
      for ( auto& appender : m_appenders ) {
        appender->log( self, level, event );
      }
    } else if ( m_root ) {
      m_root->log( level, event );
    }
  }
}

void Logger::setFormatter( LogFormatter::SPtr val )
{
  MutexType::Lock lock { m_mutex };
  m_formatter = val;

  for ( const auto& appender : m_appenders ) {
    MutexType::Lock scopeLock { appender->m_mutex };
    if ( !appender->m_hasFormatter ) {
      appender->m_formatter = m_formatter;
    }
  }
}

void Logger::setFormatter( std::string_view val )
{
  sylar::LogFormatter::SPtr newVal { std::make_shared<sylar::LogFormatter>( val ) };
  if ( newVal->isError() ) {
    std::cout << "Logger setFormatter name = " << m_name << " value = " << val << " invalid formatter" << std::endl;
    return;
  }
  m_formatter = newVal;
}

LogFormatter::SPtr Logger::getFormatter()
{
  MutexType::Lock lock { m_mutex };
  return m_formatter;
}

std::string Logger::toYamlString()
{
  MutexType::Lock lock { m_mutex };
  YAML::Node node;
  node["name"] = m_name;
  if ( m_level != LogLevel::UNKNOW ) {
    node["level"] = LogLevel::ToString( m_level );
  }
  if ( m_formatter ) {
    node["formatter"] = m_formatter->getPattern();
  }

  for ( const auto& appender : m_appenders ) {
    node["appenders"].push_back( YAML::Load( appender->toYamlString() ) );
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

void Logger::debug( LogEvent::SPtr event )
{
  log( LogLevel::DEBUG, event );
}

void Logger::info( LogEvent::SPtr event )
{
  log( LogLevel::INFO, event );
}

void Logger::warn( LogEvent::SPtr event )
{
  log( LogLevel::WARN, event );
}

void Logger::error( LogEvent::SPtr event )
{
  log( LogLevel::ERROR, event );
}

void Logger::fatal( LogEvent::SPtr event )
{
  log( LogLevel::FATAL, event );
}

FileLogAppender::FileLogAppender( std::string_view filename ) : m_filename { filename }
{
  reopen();
}

bool FileLogAppender::reopen()
{
  MutexType::Lock lock { m_mutex };
  if ( m_filestream ) {
    m_filestream.close();
  }
  m_filestream.open( m_filename );
  return m_filestream.is_open();
}

void FileLogAppender::log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event )
{
  if ( level >= m_level ) {
    std::uint64_t now = std::time( 0 );
    if ( now != m_lastTime ) {
      reopen();
      m_lastTime = now;
    }
    MutexType::Lock lock( m_mutex );
    if ( !( m_filestream << m_formatter->format( logger, level, event ) ) ) {
      std::cout << "error" << std::endl;
    }
  }
}

std::string FileLogAppender::toYamlString()
{
  MutexType::Lock lock { m_mutex };
  YAML::Node node;
  node["type"] = "FileLogAppender";
  node["file"] = m_filename;
  if ( m_level != LogLevel::UNKNOW ) {
    node["level"] = LogLevel::ToString( m_level );
  }
  if ( m_formatter ) {
    node["formatter"] = m_formatter->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

void StdoutLogAppender::log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event )
{
  MutexType::Lock lock { m_mutex };
  if ( level >= m_level ) {
    std::cout << m_formatter->format( logger, level, event );
  }
}

std::string StdoutLogAppender::toYamlString()
{
  YAML::Node node;
  node["type"] = "StdoutLogAppender";
  if ( m_level != LogLevel::UNKNOW ) {
    node["level"] = LogLevel::ToString( m_level );
  }
  if ( m_formatter ) {
    node["formatter"] = m_formatter->getPattern();
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

LogFormatter::LogFormatter( std::string_view pattern ) : m_pattern { pattern }
{
  init();
}

std::string LogFormatter::format( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event )
{
  std::stringstream ss;
  for ( auto& item : m_items ) {
    item->format( ss, logger, level, event );
  }
  return ss.str();
}

/**
 * @brief 简单的状态机判断，提取pattern中的常规字符和模式字符
 * 解析的过程就是从头到尾遍历，根据状态标志决定当前字符是常规字符还是模式字符
 * 一共有两种状态，即正在解析常规字符和正在解析模板转义字符
 * 比较麻烦的是%%d，后面可以接一对大括号指定时间格式，比如%d{%Y-%m-%d %H:%M:%S}，这个状态需要特殊处理
 *  一旦状态出错就停止解析，并设置错误标志，未识别的pattern转义字符也算出错
 */
void LogFormatter::init()
{
  // 按顺序存储解析到的pattern项
  // 每个pattern包括一个整数类型和一个字符串，类型为REGULAR_SYMBOL表示该pattern是常规字符串，为INNER_SYMBOL表示该pattern需要转义
  // 日期格式单独用下面的dataformat存储
  std::vector<std::pair<int, std::string>> patterns;
  // 临时存储常规字符串
  std::string tmp;
  // 日期格式字符串，默认把位于%d后面的大括号对里的全部字符当作格式字符，不校验是否符合日志格式
  std::string dateFormat;
  // 是否正在解析常规字符，初始时为true
  bool parsingString { true };

  // 默认 "%d{%Y-%m-%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"
  size_t i = 0;
  while ( i < m_pattern.size() ) {
    std::string c = std::string( 1, m_pattern[i] );
    if ( c == "%" ) {
      if ( parsingString ) {
        if ( !tmp.empty() ) {
          patterns.push_back( std::make_pair( REGULAR_SYMBOL, tmp ) );
        }
        tmp.clear();
        parsingString = false; // 在解析普通字符时遇到%, 表示开始解析模板字符
        ++i;
      } else {
        patterns.push_back( std::make_pair( INNER_SYMBOL, c ) );
        parsingString = true; // 在解析模板字符时遇到%, 表示这里是一个转义
        ++i;
        continue;
      }
    } else {                 // c不是%
      if ( parsingString ) { // 继续解析普通字符，解析得到的字符串作为一个普通字符串加入到patterns中
        tmp += c;
        ++i;
        continue;
      } else { // 模板字符，直接添加到patterns中，添加完成后，状态变为解析普通字符，%d做特殊处理
        patterns.push_back( std::make_pair( INNER_SYMBOL, c ) );
        parsingString = true;

        // 之后是对%d的特殊处理，如果%d后面跟了一对大括号，那么把大括号里面的内容提取出来作为dateFormat
        if ( c != "d" ) {
          ++i;
          continue;
        }

        ++i;
        if ( i < m_pattern.size() && m_pattern[i] != '{' ) {
          continue;
        }

        ++i;
        while ( i < m_pattern.size() && m_pattern[i] != '}' ) {
          dateFormat.push_back( m_pattern[i] );
          ++i;
        }

        if ( m_pattern[i] != '}' ) { // %d 后的大括号没闭合
          std::cout << "[ERROR] LogFormatter::init() pattern: [" << m_pattern << "] '{' not closed" << std::endl;
          m_error = true;
          break;
        }
        ++i;
        continue;
      }
    }
  }

  // 模板解析结束之后剩余的常规字符也要算进去
  if ( !tmp.empty() ) {
    patterns.push_back( std::make_pair( REGULAR_SYMBOL, tmp ) );
    tmp.clear();
  }

  static std::map<std::string, std::function<FormatItem::SPtr( std::string_view str )>> s_format_items
  {
#define XX( str, C )                                                                                               \
  { #str, []( std::string_view fmt ) { return FormatItem::SPtr( std::make_shared<C>( fmt ) ); } },
    XX( m, MessageFormatItem )    // m:消息
    XX( p, LevelFormatItem )      // p:日志级别
    XX( r, ElapseFormatItem )     // r:累计毫秒数
    XX( c, NameFormatItem )       // c:日志名称
    XX( t, ThreadIdFormatItem )   // t:线程id
    XX( n, NewLineFormatItem )    // n:换行
    XX( f, FilenameFormatItem )   // f:文件名
    XX( l, LineFormatItem )       // l:行号
    XX( T, TabFormatItem )        // T:Tab
    XX( F, FiberIdFormatItem )    // F:协程id
    XX( N, ThreadNameFormatItem ) // N:线程名称
#undef XX
  };

  for ( const auto& [symbolType, pattern] : patterns ) {
    if ( REGULAR_SYMBOL == symbolType ) {
      m_items.push_back( std::make_shared<StringFormatItem>( pattern ) );
    } else if ( pattern == "d" ) {
      m_items.push_back( std::make_shared<DateTimeFormatItem>( dateFormat ) );
    } else {
      const auto& iter = s_format_items.find( pattern );
      if ( iter == s_format_items.end() ) {
        std::cout << "[ERROR] LogFormatter::init()  pattern: [" << m_pattern << "] "
                  << "unknown format item: " << pattern << std::endl;
        m_error = true;
      } else {
        m_items.push_back( iter->second( pattern ) );
      }
    }
  }
}

LoggerManager::LoggerManager()
{
  m_root.reset( new Logger );
  m_root->addAppender( std::make_shared<StdoutLogAppender>() );

  init();
}

/**
 * @brief 如果指定名称的日志器未找到，那会就新创建一个，但是新创建的Logger是不带Appender的，
 * 需要手动添加Appender
 */
Logger::SPtr LoggerManager::getLogger( const std::string& name )
{
  MutexType::Lock lock { m_mutex };
  auto it { m_loggers.find( name ) };
  if ( it != m_loggers.end() ) {
    return it->second;
  }
  Logger::SPtr logger { std::make_shared<Logger>( name ) };
  logger->m_root = m_root;
  m_loggers[name] = logger;
  return logger;
}

/**
 * @brief 日志输出器配置结构体定义
 */
struct LogAppenderDefine
{
  int type { 0 }; // 1 File, 2 Stdout
  LogLevel::Level level { LogLevel::UNKNOW };
  std::string formatter;
  std::string file;

  bool operator==( const LogAppenderDefine& other ) const
  {
    return type == other.type && level == other.level && formatter == other.formatter && file == other.file;
  }
};

/**
 * @brief 日志器配置结构体定义
 */
struct LogDefine
{
  std::string name;
  LogLevel::Level level { LogLevel::UNKNOW };
  std::string formatter;
  std::vector<LogAppenderDefine> appenders;

  bool operator==( const LogDefine& other ) const
  {
    return name == other.name && level && other.level && formatter == other.formatter
           && appenders == other.appenders;
  }

  bool operator<( const LogDefine& other ) const { return name < other.name; }
};

template<>
class LexicalCast<std::string, std::set<LogDefine>>
{
public:
  std::set<LogDefine> operator()( const std::string& val )
  {
    YAML::Node node = YAML::Load( val );
    std::set<LogDefine> st;
    for ( std::size_t i = 0; i < node.size(); ++i ) {
      auto curNode = node[i];
      if ( !curNode["name"].IsDefined() ) {
        std::cout << "log config error: name is null, " << curNode << std::endl;
        continue;
      }

      LogDefine ld;
      ld.name = curNode["name"].as<std::string>();
      ld.level = LogLevel::FromString( curNode["level"].IsDefined() ? curNode["level"].as<std::string>() : "" );
      if ( curNode["formatter"].IsDefined() ) {
        ld.formatter = curNode["formatter"].as<std::string>();
      }

      if ( curNode["appenders"].IsDefined() ) {
        std::cout << "==" << ld.name << " = " << curNode["appenders"].size() << std::endl;
        for ( std::size_t j = 0; j < curNode["appenders"].size(); ++j ) {
          auto appenderNode = curNode["appenders"][j];
          if ( !appenderNode["type"].IsDefined() ) {
            std::cout << "log config error: appender type is null, " << appenderNode << std::endl;
            continue;
          }

          std::string type = appenderNode["type"].as<std::string>();
          LogAppenderDefine lad;
          if ( "FileLogAppender" == type ) {
            lad.type = 1;
            if ( !curNode["file"].IsDefined() ) {
              std::cout << "log config error: fileappender file is null, " << appenderNode << std::endl;
              continue;
            }
            lad.file = appenderNode["file"].as<std::string>();
            if ( appenderNode["formatter"].IsDefined() ) {
              lad.formatter = appenderNode["formatter"].as<std::string>();
            }
          } else if ( "StdoutLogAppender" == type ) {
            lad.type = 2;
            if ( appenderNode["formatter"].IsDefined() ) {
              lad.formatter = appenderNode["formatter"].as<std::string>();
            }
          } else {
            std::cout << "log config error: appender type is invalid, " << appenderNode << std::endl;
            continue;
          }

          ld.appenders.push_back( lad );
        }
      }
      std::cout << "---" << ld.name << " - " << ld.appenders.size() << std::endl;
      st.insert( ld );
    }
    return st;
  }
};

template<>
class LexicalCast<std::set<LogDefine>, std::string>
{
public:
  std::string operator()( const std::set<LogDefine>& val )
  {
    YAML::Node node;
    for ( const auto& logDefine : val ) {
      YAML::Node curNode;
      curNode["name"] = logDefine.name;
      if ( logDefine.level != LogLevel::UNKNOW ) {
        curNode["level"] = LogLevel::ToString( logDefine.level );
      }
      if ( logDefine.formatter.empty() ) {
        curNode["formatter"] = logDefine.formatter;
      }

      for ( const auto& appender : logDefine.appenders ) {
        YAML::Node appenderNode;
        if ( 1 == appender.type ) {
          appenderNode["type"] = "FileLogAppender";
          appenderNode["file"] = appender.file;
        } else if ( 2 == appender.type ) {
          appenderNode["type"] = "StdoutLogAppender";
        }

        if ( appender.level != LogLevel::UNKNOW ) {
          appenderNode["level"] = LogLevel::ToString( appender.level );
        }

        if ( !appender.formatter.empty() ) {
          appenderNode["formatter"] = appender.formatter;
        }

        curNode["appenders"].push_back( appenderNode );
      }
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

sylar::ConfigVar<std::set<LogDefine>>::SPtr g_log_defines {
  sylar::Config::Lookup( "logs", std::set<LogDefine> {}, "logs config" ) };

struct LogIniter
{
  LogIniter()
  {
    g_log_defines->addListener( []( const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value ) {
      for ( const auto& logDefine : new_value ) {
        auto it = old_value.find( logDefine );
        sylar::Logger::SPtr logger;
        if ( it == old_value.end() || !( *it == logDefine ) ) {
          // 新增logger
          logger = SYLAR_LOG_NAME( logDefine.name );
        }
        logger->setLevel( logDefine.level );
        if ( !logDefine.formatter.empty() ) {
          logger->setFormatter( logDefine.formatter );
        }

        logger->clearAppenders();
        for ( const auto& appender : logDefine.appenders ) {
          sylar::LogAppender::SPtr newAppender;
          if ( appender.type == 1 ) {
            newAppender = std::make_shared<FileLogAppender>( appender.file );
          } else if ( appender.type == 2 ) {
            newAppender = std::make_shared<StdoutLogAppender>();
          }

          if ( newAppender ) {
            newAppender->setLevel( appender.level );
            logger->addAppender( newAppender );
          }
        }
      }

      // 以配置文件为主，如果程序里定义了配置文件中未定义的logger，那么把程序里定义的logger设置成无效
      for ( const auto& appender : old_value ) {
        auto it = new_value.find( appender );
        if ( it == new_value.end() ) {
          auto logger = SYLAR_LOG_NAME( appender.name );
          logger->setLevel( (LogLevel::Level)100 );
          logger->clearAppenders();
        }
      }
    } );
  }
};

/**
 * @brief 在main函数之前注册配置更改的回调函数, 用于在更新配置时将log相关的配置加载到Config
 */
static LogIniter __log_init;

std::string LoggerManager::toYamlString()
{
  MutexType::Lock lock { m_mutex };
  YAML::Node node;
  for ( const auto& [_, logger] : m_loggers ) {
    node.push_back( YAML::Load( logger->toYamlString() ) );
  }
  std::stringstream ss;
  ss << node;
  return ss.str();
}

void LoggerManager::init() {}

} // namespace sylar