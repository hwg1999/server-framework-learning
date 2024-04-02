#include "log.h"
#include "config.h"
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
#include <sstream>
#include <string>
#include <tuple>
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

LogLevel::Level LogLevel::FromString( const std::string& str )
{
#define XX( level, val )                                                                                           \
  if ( str == #val ) {                                                                                             \
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
                    std::uint32_t thread_id,
                    std::uint32_t fiber_id,
                    std::uint64_t time,
                    const std::string& thread_name )
  : m_file( file )
  , m_line( line )
  , m_elapse( elapse )
  , m_threadId( thread_id )
  , m_fiberId( fiber_id )
  , m_time( time )
  , m_threadName( thread_name )
  , m_logger( logger )
  , m_level( level )
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

LogEventWrap::LogEventWrap( LogEvent::SPtr event ) : m_event( event ) {}

LogEventWrap::~LogEventWrap()
{
  m_event->getLogger()->log( m_event->getLevel(), m_event );
}

class MessageFormatItem : public LogFormatter::FormatItem
{
public:
  MessageFormatItem( const std::string& str = "" ) {}
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
  LevelFormatItem( const std::string& str = "" ) {}
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
  ElapseFormatItem( const std::string& str = "" ) {}
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
  NameFormatItem( const std::string& str = "" ) {}
  void format( std::ostream& os, Logger::SPtr logger, LogLevel::Level level, LogEvent::SPtr event ) override
  {
    os << logger->getName();
  }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem
{
public:
  ThreadIdFormatItem( const std::string& str = "" ) {}
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
  FiberIdFormatItem( const std::string& str = "" ) {}
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
  ThreadNameFormatItem( const std::string& str = "" ) {}
  void format( std::ostream& os, Logger::SPtr logger, LogLevel::Level level, LogEvent::SPtr event ) override
  {
    os << event->getThreadName();
  }
};

class DateTimeFormatItem : public LogFormatter::FormatItem
{
public:
  DateTimeFormatItem( const std::string& format = "%Y:%m:%d %H:%M:%S" ) : m_format( format )
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
  FilenameFormatItem( const std::string& str = "" ) {}
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
  LineFormatItem( const std::string& str = "" ) {}
  void format( std::ostream& os, Logger::SPtr logger, LogLevel::Level level, LogEvent::SPtr event ) override
  {
    os << event->getLine();
  }
};

class NewLineFormatItem : public LogFormatter::FormatItem
{
public:
  NewLineFormatItem( const std::string& str = "" ) {}
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
  StringFormatItem( const std::string& str ) : m_string( str ) {}
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
  TabFormatItem( const std::string& str = "" ) {}
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

Logger::Logger( const std::string& name ) : m_name( name ), m_level( LogLevel::DEBUG )
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

void Logger::log( LogLevel::Level level, LogEvent::SPtr event )
{
  if ( level >= m_level ) {
    auto self = shared_from_this();
    MutexType::Lock lock( m_mutex );
    if ( !m_appenders.empty() ) {
      for ( auto& i : m_appenders ) {
        i->log( self, level, event );
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
void Logger::setFormatter( const std::string& val )
{
  sylar::LogFormatter::SPtr new_val( std::make_shared<sylar::LogFormatter>( val ) );
  if ( new_val->isError() ) {
    std::cout << "Logger setFormatter name = " << m_name << " value = " << val << " invalid formatter" << std::endl;
    return;
  }
  m_formatter = new_val;
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

FileLogAppender::FileLogAppender( const std::string& filename ) : m_filename( filename )
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

LogFormatter::LogFormatter( const std::string& pattern ) : m_pattern( pattern )
{
  init();
}

std::string LogFormatter::format( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event )
{
  std::stringstream ss;
  for ( auto& i : m_items ) {
    i->format( ss, logger, level, event );
  }
  return ss.str();
}

void LogFormatter::init()
{
  std::vector<std::tuple<std::string, std::string, int>> vec;
  std::string nstr;
  for ( size_t i = 0; i < m_pattern.size(); ++i ) {
    if ( m_pattern[i] != '%' ) {
      nstr.append( 1, m_pattern[i] );
      continue;
    }

    if ( ( i + 1 ) < m_pattern.size() ) {
      if ( m_pattern[i + 1] == '%' ) {
        nstr.append( 1, '%' );
        continue;
      }
    }

    size_t n = i + 1;
    int fmt_status = REGULAR_SYMBOL;
    size_t fmt_begin = 0;

    std::string str;
    std::string fmt;
    while ( n < m_pattern.size() ) {
      if ( !fmt_status && ( !isalpha( m_pattern[n] ) && m_pattern[n] != '{' && m_pattern[n] != '}' ) ) {
        str = m_pattern.substr( i + 1, n - i - 1 );
        break;
      }

      if ( fmt_status == REGULAR_SYMBOL && m_pattern[n] == '{' ) {
        str = m_pattern.substr( i + 1, n - i - 1 );
        fmt_status = INNER_SYMBOL; // 解析格式
        fmt_begin = n;
        ++n;
        continue;
      } else if ( fmt_status == INNER_SYMBOL && m_pattern[n] == '}' ) {
        fmt = m_pattern.substr( fmt_begin + 1, n - fmt_begin - 1 );
        fmt_status = REGULAR_SYMBOL;
        ++n;
        break;
      }

      ++n;

      if ( n == m_pattern.size() && str.empty() ) {
        str = m_pattern.substr( i + 1 );
      }
    }

    if ( fmt_status == REGULAR_SYMBOL ) {
      if ( !nstr.empty() ) {
        vec.push_back( std::make_tuple( nstr, std::string(), 0 ) );
        nstr.clear();
      }
      vec.push_back( std::make_tuple( str, fmt, INNER_SYMBOL ) );
      i = n - 1;
    } else if ( fmt_status == INNER_SYMBOL ) {
      std::cout << "pattern parse error: " << m_pattern << " - " << m_pattern.substr( i ) << std::endl;
      m_error = true;
      vec.push_back( std::make_tuple( "<<pattern_error>>", fmt, REGULAR_SYMBOL ) );
    }
  }

  if ( !nstr.empty() ) {
    vec.push_back( std::make_tuple( nstr, "", REGULAR_SYMBOL ) );
  }

  static std::map<std::string, std::function<FormatItem::SPtr( const std::string& str )>> s_format_items {
#define XX( str, C )                                                                                               \
  { #str, []( const std::string& fmt ) { return FormatItem::SPtr( std::make_shared<C>( fmt ) ); } },
    XX( m, MessageFormatItem )    // m:消息
    XX( p, LevelFormatItem )      // p:日志级别
    XX( r, ElapseFormatItem )     // r:累计毫秒数
    XX( c, NameFormatItem )       // c:日志名称
    XX( t, ThreadIdFormatItem )   // t:线程id
    XX( n, NewLineFormatItem )    // n:换行
    XX( d, DateTimeFormatItem )   // d:时间
    XX( f, FilenameFormatItem )   // f:文件名
    XX( l, LineFormatItem )       // l:行号
    XX( T, TabFormatItem )        // T:Tab
    XX( F, FiberIdFormatItem )    // F:协程id
    XX( N, ThreadNameFormatItem ) // N:线程名称
#undef XX
  };

  for ( auto& i : vec ) {
    if ( std::get<FORMAT_STATUS>( i ) == REGULAR_SYMBOL ) {
      m_items.push_back( std::make_shared<StringFormatItem>( std::get<RAW_STRING>( i ) ) );
    } else {
      auto it = s_format_items.find( std::get<RAW_STRING>( i ) );
      if ( it == s_format_items.end() ) {
        m_items.push_back( FormatItem::SPtr(
          std::make_shared<StringFormatItem>( "<<error_format %" + std::get<RAW_STRING>( i ) + ">>\n" ) ) );
        m_error = true;
      } else {
        m_items.push_back( it->second( std::get<FORMAT_STRING>( i ) ) );
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

struct LogAppenderDefine
{
  int type { 0 };
  LogLevel::Level level { LogLevel::UNKNOW };
  std::string formatter;
  std::string file;

  bool operator==( const LogAppenderDefine& other ) const
  {
    return type == other.type && level == other.level && formatter == other.formatter && file == other.file;
  }
};

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