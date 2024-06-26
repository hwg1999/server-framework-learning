#include "sylar/http/http_servlet.h"
#include "sylar/http/http.h"
#include <fnmatch.h>
#include <map>
#include <memory>
#include <string>
#include <utility>

namespace sylar {
namespace http {

FunctionServlet::FunctionServlet( callback cb ) : Servlet { "FunctionServlet" }, m_cb { cb } {}

int32_t FunctionServlet::handle( sylar::http::HttpRequest::SPtr request,
                                 sylar::http::HttpResponse::SPtr response,
                                 sylar::http::HttpSession::SPtr session )
{
  return m_cb( request, response, session );
}

ServletDispatch::ServletDispatch()
  : Servlet { "ServletDispatch" }, default_ { std::make_shared<NotFoundServlet>( "sylar/1.0" ) }
{}

int32_t ServletDispatch::handle( sylar::http::HttpRequest::SPtr request,
                                 sylar::http::HttpResponse::SPtr response,
                                 sylar::http::HttpSession::SPtr session )
{
  auto slt = getMatchedServlet( request->getPath() );
  if ( slt ) {
    slt->handle( request, response, session );
  }
  return 0;
}

void ServletDispatch::addServlet( const std::string& uri, Servlet::SPtr slt )
{
  RWMutexType::WriteLock lock { mutex_ };
  datas_[uri] = std::make_shared<HoldServletCreator>( slt );
}

void ServletDispatch::addServletCreator( const std::string& uri, IServletCreator::SPtr creator )
{
  RWMutexType::WriteLock lock { mutex_ };
  datas_[uri] = creator;
}

void ServletDispatch::addGlobServletCreator( const std::string& uri, IServletCreator::SPtr creator )
{
  RWMutexType::WriteLock lock { mutex_ };
  for ( auto it = globs_.begin(); it != globs_.end(); ++it ) {
    if ( it->first == uri ) {
      globs_.erase( it );
      break;
    }
  }

  globs_.push_back( std::make_pair( uri, creator ) );
}

void ServletDispatch::addServlet( const std::string& uri, FunctionServlet::callback cb )
{
  RWMutexType::WriteLock lock { mutex_ };
  datas_[uri] = std::make_shared<HoldServletCreator>( std::make_shared<FunctionServlet>( cb ) );
}

void ServletDispatch::addGlobServlet( const std::string& uri, Servlet::SPtr slt )
{
  RWMutexType::WriteLock lock { mutex_ };
  for ( auto it = globs_.begin(); it != globs_.end(); ++it ) {
    if ( it->first == uri ) {
      globs_.erase( it );
      break;
    }
  }

  globs_.push_back( std::make_pair( uri, std::make_shared<HoldServletCreator>( slt ) ) );
}

void ServletDispatch::addGlobServlet( const std::string& uri, FunctionServlet::callback cb )
{
  return addGlobServlet( uri, std::make_shared<FunctionServlet>( cb ) );
}

void ServletDispatch::delServlet( const std::string& uri )
{
  RWMutexType::WriteLock lock { mutex_ };
  datas_.erase( uri );
}

void ServletDispatch::delGlobServlet( const std::string& uri )
{
  RWMutexType::WriteLock lock { mutex_ };
  for ( auto it = globs_.begin(); it != globs_.end(); ++it ) {
    if ( it->first == uri ) {
      globs_.erase( it );
      break;
    }
  }
}

Servlet::SPtr ServletDispatch::getServlet( const std::string& uri )
{
  RWMutexType::ReadLock lock { mutex_ };
  auto it = datas_.find( uri );
  return it == datas_.end() ? nullptr : it->second->get();
}

Servlet::SPtr ServletDispatch::getGlobServlet( const std::string& uri )
{
  RWMutexType::ReadLock lock { mutex_ };
  for ( auto it = globs_.begin(); it != globs_.end(); ++it ) {
    if ( it->first == uri ) {
      return it->second->get();
    }
  }

  return nullptr;
}

Servlet::SPtr ServletDispatch::getMatchedServlet( const std::string& uri )
{
  RWMutexType::ReadLock lock { mutex_ };
  auto mit = datas_.find( uri );
  if ( mit != datas_.end() ) {
    return mit->second->get();
  }

  for ( auto it = globs_.begin(); it != globs_.end(); ++it ) {
    if ( !fnmatch( it->first.c_str(), uri.c_str(), 0 ) ) {
      return it->second->get();
    }
  }

  return default_;
}

void ServletDispatch::listAllServletCreator( std::map<std::string, IServletCreator::SPtr>& infos )
{
  RWMutexType::ReadLock lock { mutex_ };
  for ( auto& i : datas_ ) {
    infos[i.first] = i.second;
  }
}

void ServletDispatch::listAllGlobServletCreator( std::map<std::string, IServletCreator::SPtr>& infos )
{
  RWMutexType::ReadLock lock { mutex_ };
  for ( auto& i : globs_ ) {
    infos[i.first] = i.second;
  }
}

NotFoundServlet::NotFoundServlet( const std::string& name ) : Servlet { "NotFoundServlet" }, name_( name )
{
  content_ = "<html><head><title>404 Not Found"
             "</title></head><body><center><h1>404 Not Found</h1></center>"
             "<hr><center>"
             + name + "</center></body></html>";
}

int32_t NotFoundServlet::handle( sylar::http::HttpRequest::SPtr request,
                                 sylar::http::HttpResponse::SPtr response,
                                 sylar::http::HttpSession::SPtr session )
{
  response->setStatus( sylar::http::HttpStatus::NOT_FOUND );
  response->setHeader( "Server", "sylar/1.0.0" );
  response->setHeader( "Content-Type", "text/html" );
  response->setBody( content_ );
  return 0;
}

}
}