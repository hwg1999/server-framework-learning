#include "log.h"
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
#include <sstream>
#include <string>
#include <tuple>

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

LogEventWrap::LogEventWrap( LogEvent::SPtr event ) : m_event( event ) {}

LogEventWrap::~LogEventWrap()
{
  m_event->getLogger()->log( m_event->getLevel(), m_event );
}

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
    os << logger->getName();
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

LogEvent::LogEvent( Logger::SPtr logger,
                    LogLevel::Level level,
                    const char* file,
                    std::int32_t line,
                    std::uint32_t elapse,
                    std::uint32_t thread_id,
                    uint32_t fiber_id,
                    std::uint64_t time )
  : m_logger( logger )
  , m_level( level )
  , m_file( file )
  , m_line( line )
  , m_elapse( elapse )
  , m_threadId( thread_id )
  , m_fiberId( fiber_id )
  , m_time( time )
{}

Logger::Logger( const std::string& name ) : m_name( name ), m_level( LogLevel::DEBUG )
{
  m_formatter.reset( new LogFormatter( "%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n" ) );
}

void Logger::addAppender( LogAppender::SPtr appender )
{
  if ( !appender->getFormatter() ) {
    appender->setFormatter( m_formatter );
  }
  m_appenders.push_back( appender );
}

void Logger::delAppender( LogAppender::SPtr appender )
{
  for ( auto it = m_appenders.begin(); it != m_appenders.end(); ++it ) {
    if ( *it == appender ) {
      m_appenders.erase( it );
      break;
    }
  }
}

void Logger::log( LogLevel::Level level, LogEvent::SPtr event )
{
  if ( level >= m_level ) {
    auto self = shared_from_this();
    for ( auto& i : m_appenders ) {
      i->log( self, level, event );
    }
  }
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
  if ( m_filestream ) {
    m_filestream.close();
  }
  m_filestream.open( m_filename );
  return m_filestream.is_open();
}

void FileLogAppender::log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event )
{
  if ( level >= m_level ) {
    m_filestream << m_formatter->format( logger, level, event );
  }
}

void StdoutLogAppender::log( std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::SPtr event )
{
  if ( level >= m_level ) {
    std::cout << m_formatter->format( logger, level, event );
  }
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
      vec.push_back( std::make_tuple( "<<pattern_error>>", fmt, REGULAR_SYMBOL ) );
    }
  }

  if ( !nstr.empty() ) {
    vec.push_back( std::make_tuple( nstr, "", REGULAR_SYMBOL ) );
  }

  static std::map<std::string, std::function<FormatItem::SPtr( const std::string& str )>> s_format_items {
#define XX( str, C )                                                                                               \
  { #str, []( const std::string& fmt ) { return FormatItem::SPtr( std::make_shared<C>( fmt ) ); } },
    XX( m, MessageFormatItem )  // m:消息
    XX( p, LevelFormatItem )    // p:日志级别
    XX( r, ElapseFormatItem )   // r:累计毫秒数
    XX( c, NameFormatItem )     // c:日志名称
    XX( t, ThreadIdFormatItem ) // t:线程id
    XX( n, NewLineFormatItem )  // n:换行
    XX( d, DateTimeFormatItem ) // d:时间
    XX( f, FilenameFormatItem ) // f:文件名
    XX( l, LineFormatItem )     // l:行号
    XX( T, TabFormatItem )      // T:tab
    XX( F, FiberIdFormatItem )  // F:协程id
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
}

Logger::SPtr LoggerManager::getLogger( const std::string& name )
{
  auto it = m_loggers.find( name );
  return it == m_loggers.end() ? m_root : it->second;
}

} // namespace sylar