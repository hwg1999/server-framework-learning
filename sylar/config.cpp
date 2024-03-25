#include "sylar/config.h"
#include "sylar/log.h"
#include <algorithm>
#include <list>
#include <sstream>
#include <utility>

namespace sylar {

ConfigVarBase::SPtr Config::LookupBase( const std::string& name )
{
  auto it = s_datas.find( name );
  return it == s_datas.end() ? nullptr : it->second;
}

static void ListAllMember( const std::string& prefix,
                           const YAML::Node& node,
                           std::list<std::pair<std::string, const YAML::Node>>& output )
{
  if ( prefix.find_first_not_of( "abcdefghikjlmnopqrstuvwxyz._012345678" ) != std::string::npos ) {
    SYLAR_LOG_ERROR( SYLAR_LOG_ROOT() ) << "Config invalid name: " << prefix << " : ";
    return;
  }
  output.push_back( std::make_pair( prefix, node ) );
  if ( node.IsMap() ) {
    for ( auto it = node.begin(); it != node.end(); ++it ) {
      ListAllMember( prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output );
    }
  }
}

void Config::LoadFromYaml( const YAML::Node& root )
{
  std::list<std::pair<std::string, const YAML::Node>> all_nodes;
  ListAllMember( "", root, all_nodes );

  for ( auto& node : all_nodes ) {
    std::string key = node.first;
    if ( key.empty() ) {
      continue;
    }

    std::transform( key.begin(), key.end(), key.begin(), ::tolower );
    ConfigVarBase::SPtr var = LookupBase( key );

    if ( var ) {
      if ( node.second.IsScalar() ) {
        var->fromString( node.second.Scalar() );
      } else {
        std::stringstream ss;
        ss << node.second;
        var->fromString( ss.str() );
      }
    }
  }
}

}
