#pragma once

#include "sylar/http/http.h"
#include "sylar/http/http_session.h"
#include "sylar/thread.h"
#include "sylar/util.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sylar {
namespace http {

class Servlet
{
public:
  using SPtr = std::shared_ptr<Servlet>;

  Servlet( const std::string& name ) : m_name { name } {}

  virtual ~Servlet() {}

  virtual int32_t handle( sylar::http::HttpRequest::SPtr request,
                          sylar::http::HttpResponse::SPtr response,
                          sylar::http::HttpSession::SPtr session )
    = 0;

  const std::string& getName() const { return m_name; }

protected:
  std::string m_name;
};

class FunctionServlet : public Servlet
{
public:
  using SPtr = std::shared_ptr<FunctionServlet>;
  using callback = std::function<int32_t( sylar::http::HttpRequest::SPtr request,
                                          sylar::http::HttpResponse::SPtr response,
                                          sylar::http::HttpSession::SPtr session )>;
  FunctionServlet( callback cb );
  virtual int32_t handle( sylar::http::HttpRequest::SPtr request,
                          sylar::http::HttpResponse::SPtr response,
                          sylar::http::HttpSession::SPtr session ) override;

private:
  callback m_cb;
};

class IServletCreator
{
public:
  using SPtr = std::shared_ptr<IServletCreator>;
  virtual ~IServletCreator() {}
  virtual Servlet::SPtr get() const = 0;
  virtual std::string getName() const = 0;
};

class HoldServletCreator : public IServletCreator
{
public:
  using SPtr = std::shared_ptr<HoldServletCreator>;

  HoldServletCreator( Servlet::SPtr slt ) : m_servlet { slt } {}

  Servlet::SPtr get() const override { return m_servlet; }

  std::string getName() const override { return m_servlet->getName(); }

private:
  Servlet::SPtr m_servlet;
};

template<typename T>
class ServletCreator : public IServletCreator
{
public:
  using SPtr = std::shared_ptr<ServletCreator>;

  ServletCreator() {}

  Servlet::SPtr get() const override { return std::make_shared<Servlet>( T {} ); }

  std::string getName() const override { return TypeToName<T>(); }
};

class ServletDispatch : public Servlet
{
public:
  using SPtr = std::shared_ptr<ServletDispatch>;
  using RWMutexType = RWMutex;

  ServletDispatch();

  virtual int32_t handle( sylar::http::HttpRequest::SPtr request,
                          sylar::http::HttpResponse::SPtr response,
                          sylar::http::HttpSession::SPtr session ) override;

  void addServlet( const std::string& uri, Servlet::SPtr slt );
  void addServlet( const std::string& uri, FunctionServlet::callback cb );
  void addGlobServlet( const std::string& uri, Servlet::SPtr slt );
  void addGlobServlet( const std::string& uri, FunctionServlet::callback cb );
  void addServletCreator( const std::string& uri, IServletCreator::SPtr creator );
  void addGlobServletCreator( const std::string& uri, IServletCreator::SPtr creator );

  template<typename T>
  void addServletCreator( const std::string& uri )
  {
    addServletCreator( uri, std::make_shared<ServletCreator<T>>() );
  }

  template<typename T>
  void addGlobServletCreator( const std::string& uri )
  {
    addGlobServletCreator( uri, std::make_shared<ServletCreator<T>>() );
  }

  void delServlet( const std::string& uri );
  void delGlobServlet( const std::string& uri );
  Servlet::SPtr getDefault() const { return default_; }
  void setDeafault( Servlet::SPtr val ) { default_ = val; }
  Servlet::SPtr getServlet( const std::string& uri );
  Servlet::SPtr getGlobServlet( const std::string& uri );
  Servlet::SPtr getMatchedServlet( const std::string& uir );

  void listAllServletCreator( std::map<std::string, IServletCreator::SPtr>& infos );
  void listAllGlobServletCreator( std::map<std::string, IServletCreator::SPtr>& infos );

private:
  RWMutexType mutex_;
  std::unordered_map<std::string, IServletCreator::SPtr> datas_;
  std::vector<std::pair<std::string, IServletCreator::SPtr>> globs_;
  Servlet::SPtr default_;
};

class NotFoundServlet : public Servlet
{
public:
  using SPtr = std::shared_ptr<NotFoundServlet>;

  NotFoundServlet( const std::string& name );

  virtual int32_t handle( sylar::http::HttpRequest::SPtr request,
                          sylar::http::HttpResponse::SPtr response,
                          sylar::http::HttpSession::SPtr session ) override;

private:
  std::string name_;
  std::string content_;
};

}
}