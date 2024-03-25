#pragma once

#include "boost/lexical_cast.hpp"
#include "log.h"
#include "yaml-cpp/yaml.h"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <exception>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace sylar {

class ConfigVarBase
{
public:
  using SPtr = std::shared_ptr<ConfigVarBase>;

  ConfigVarBase( const std::string& name, const std::string& description = "" )
    : m_name( name ), m_description( description )
  {
    std::transform( name.begin(), name.end(), m_name.begin(), ::tolower );
  }

  virtual ~ConfigVarBase() {}

  const std::string& getName() const { return m_name; }
  const std::string& getDescription() const { return m_description; }

  virtual std::string toString() = 0;
  virtual bool fromString( const std::string& val ) = 0;

protected:
  std::string m_name;
  std::string m_description;
};

template<typename From, typename To>
class LexicalCast
{
public:
  To operator()( const From& v ) { return boost::lexical_cast<To>( v ); }
};

template<typename T>
class LexicalCast<std::string, std::vector<T>>
{
public:
  std::vector<T> operator()( const std::string& val )
  {
    YAML::Node node = YAML::Load( val );
    std::vector<T> vec;
    std::stringstream ss;
    for ( std::size_t i = 0; i < node.size(); ++i ) {
      ss.str( "" );
      ss << node[i];
      vec.push_back( LexicalCast<std::string, T> {}( ss.str() ) );
    }
    return vec;
  }
};

template<typename T>
class LexicalCast<std::vector<T>, std::string>
{
public:
  std::string operator()( const std::vector<T>& val )
  {
    YAML::Node node;
    for ( const auto& v : val ) {
      node.push_back( YAML::Load( LexicalCast<T, std::string> {}( v ) ) );
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template<typename T>
class LexicalCast<std::string, std::list<T>>
{
public:
  std::list<T> operator()( const std::string& val )
  {
    YAML::Node node = YAML::Load( val );
    std::list<T> vec;
    std::stringstream ss;
    for ( std::size_t i = 0; i < node.size(); ++i ) {
      ss.str( "" );
      ss << node[i];
      vec.push_back( LexicalCast<std::string, T> {}( ss.str() ) );
    }
    return vec;
  }
};

template<typename T>
class LexicalCast<std::list<T>, std::string>
{
public:
  std::string operator()( const std::list<T>& val )
  {
    YAML::Node node;
    for ( const auto& v : val ) {
      node.push_back( YAML::Load( LexicalCast<T, std::string> {}( v ) ) );
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template<typename T>
class LexicalCast<std::string, std::set<T>>
{
public:
  std::set<T> operator()( const std::string& val )
  {
    YAML::Node node = YAML::Load( val );
    std::set<T> st;
    std::stringstream ss;
    for ( std::size_t i = 0; i < node.size(); ++i ) {
      ss.str( "" );
      ss << node[i];
      st.insert( LexicalCast<std::string, T> {}( ss.str() ) );
    }
    return st;
  }
};

template<typename T>
class LexicalCast<std::set<T>, std::string>
{
public:
  std::string operator()( const std::set<T>& val )
  {
    YAML::Node node;
    for ( const auto& v : val ) {
      node.push_back( YAML::Load( LexicalCast<T, std::string> {}( v ) ) );
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template<typename T>
class LexicalCast<std::string, std::unordered_set<T>>
{
public:
  std::unordered_set<T> operator()( const std::string& val )
  {
    YAML::Node node = YAML::Load( val );
    std::unordered_set<T> st;
    std::stringstream ss;
    for ( std::size_t i = 0; i < node.size(); ++i ) {
      ss.str( "" );
      ss << node[i];
      st.insert( LexicalCast<std::string, T> {}( ss.str() ) );
    }
    return st;
  }
};

template<typename T>
class LexicalCast<std::unordered_set<T>, std::string>
{
public:
  std::string operator()( const std::unordered_set<T>& val )
  {
    YAML::Node node;
    for ( const auto& v : val ) {
      node.push_back( YAML::Load( LexicalCast<T, std::string> {}( v ) ) );
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template<typename T>
class LexicalCast<std::string, std::map<std::string, T>>
{
public:
  std::map<std::string, T> operator()( const std::string& val )
  {
    YAML::Node node = YAML::Load( val );
    std::map<std::string, T> mp;
    std::stringstream ss;
    for ( auto it = node.begin(); it != node.end(); ++it ) {
      ss.str( "" );
      ss << it->second;
      mp.insert( std::make_pair( it->first.Scalar(), LexicalCast<std::string, T> {}( ss.str() ) ) );
    }
    return mp;
  }
};

template<typename T>
class LexicalCast<std::map<std::string, T>, std::string>
{
public:
  std::string operator()( const std::map<std::string, T>& val )
  {
    YAML::Node node;
    for ( const auto& v : val ) {
      node[v.first] = YAML::Load( LexicalCast<T, std::string> {}( v.second ) );
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template<typename T>
class LexicalCast<std::string, std::unordered_map<std::string, T>>
{
public:
  std::unordered_map<std::string, T> operator()( const std::string& val )
  {
    YAML::Node node = YAML::Load( val );
    std::unordered_map<std::string, T> mp;
    std::stringstream ss;
    for ( auto it = node.begin(); it != node.end(); ++it ) {
      ss.str( "" );
      ss << it->second;
      mp.insert( std::make_pair( it->first.Scalar(), LexicalCast<std::string, T> {}( ss.str() ) ) );
    }
    return mp;
  }
};

template<typename T>
class LexicalCast<std::unordered_map<std::string, T>, std::string>
{
public:
  std::string operator()( const std::unordered_map<std::string, T>& val )
  {
    YAML::Node node;
    for ( const auto& v : val ) {
      node[v.first] = YAML::Load( LexicalCast<T, std::string> {}( v.second ) );
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
  }
};

template<typename T, typename FromStr = LexicalCast<std::string, T>, typename ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase
{
public:
  using SPtr = std::shared_ptr<ConfigVar>;
  using on_change_cb = std::function<void( const T& old_valuem, const T& new_value )>;
  ConfigVar( const std::string& name, const T& default_value, const std::string& description = "" )
    : ConfigVarBase( name, description ), m_val( default_value )
  {}

  std::string toString() override
  {
    try {
      return ToStr {}( m_val );
    } catch ( std::exception& e ) {
      SYLAR_LOG_ERROR( SYLAR_LOG_ROOT() )
        << "ConfigVar::toString exception " << e.what() << " convert: " << typeid( m_val ).name() << " to string";
    }

    return "";
  }

  bool fromString( const std::string& val ) override
  {
    try {
      setValue( FromStr {}( val ) );
    } catch ( std::exception& e ) {
      SYLAR_LOG_ERROR( SYLAR_LOG_ROOT() )
        << "ConfigVar::fromString exception " << e.what() << " convert: string to" << typeid( m_val ).name();
    }

    return false;
  }

  const T getValue() const { return m_val; }

  void setValue( const T& val )
  {
    if ( val == m_val ) {
      return;
    }

    for ( const auto& cb : m_cbs ) {
      cb.second( m_val, val );
    }
    m_val = val;
  }

  void addListener( std::uint64_t key, on_change_cb cb ) { m_cbs[key] = cb; }

  void delListener( std::uint64_t key ) { m_cbs.emplace( key ); }

  void getListener( std::uint64_t key )
  {
    auto it = m_cbs.find( key );
    return it == m_cbs.end() ? nullptr : it->second;
  }

  void clearListener() { m_cbs.clear(); }

private:
  T m_val;
  std::map<std::uint64_t, on_change_cb> m_cbs;
};

class Config
{
public:
  using ConfigVarMap = std::map<std::string, ConfigVarBase::SPtr>;

  template<typename T>
  static typename ConfigVar<T>::SPtr Lookup( const std::string& name,
                                             const T& default_value,
                                             const std::string& description = "" )
  {
    auto tmp = Lookup<T>( name );
    if ( tmp ) {
      SYLAR_LOG_INFO( SYLAR_LOG_ROOT() ) << "Lookup name = " << name << " exists";
      return tmp;
    }

    if ( name.find_first_not_of( "abcdefghikjlmnopqrstuvwxyz._012345678" ) != std::string::npos ) {
      SYLAR_LOG_ERROR( SYLAR_LOG_ROOT() ) << "Lookup name invalid" << name;
      throw std::invalid_argument( name );
    }

    s_datas[name] = std::make_shared<ConfigVar<T>>( name, default_value, description );
    return std::dynamic_pointer_cast<ConfigVar<T>>( s_datas[name] );
  }

  template<typename T>
  static typename ConfigVar<T>::SPtr Lookup( const std::string& name )
  {
    auto it = s_datas.find( name );
    if ( it == s_datas.end() ) {
      return nullptr;
    }
    return std::dynamic_pointer_cast<ConfigVar<T>>( it->second );
  }

  static void LoadFromYaml( const YAML::Node& root );
  static ConfigVarBase::SPtr LookupBase( const std::string& name );

private:
  static inline ConfigVarMap s_datas;
};

}