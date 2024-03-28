#include "CollectionRegistry.h"
#include "Keywords.h"
#include <strutil.h>

using namespace BPMNOS;

Collection::Collection(const std::string& string)
  : string(strutil::trim_copy(string))
{
  // get values
  if ( !strutil::starts_with(string,"[") || !strutil::ends_with(string,"]") ) {
    throw std::runtime_error("Collection: string must start with '[' and must end with ']'");
  }
  // determine list of comma separated values
  auto list = string.substr(1,string.size()-2);
  if ( list.empty() ) return;
    
  auto elements = strutil::split( list, "," );
  for ( auto& element : elements ) {
    strutil::trim(element);
    if ( strutil::starts_with(element,"\"") && strutil::ends_with(element,"\"") ) {
      values.push_back( BPMNOS::to_number( element.substr(1,element.size()-2), STRING ) );
    }
    else if ( element == Keyword::False ) {
      values.push_back( BPMNOS::to_number( false , BOOLEAN) );
    }
    else if ( element == Keyword::True ) {
      values.push_back( BPMNOS::to_number( true , BOOLEAN) );
    }
    else if ( element == Keyword::Undefined ) {
      values.push_back( std::nullopt );
    }
    else {
      try {
        // try to convert to number
        values.push_back( BPMNOS::stod(element) );
      }
      catch(...) {
        throw std::runtime_error("Collection: illegal value '" + element + "' in '" + string + "'");
      }
    }
  }
}

CollectionRegistry::CollectionRegistry() {
  // register empty collection with index 0
  (*this)("[]");
}

const Collection&  CollectionRegistry::operator[](long unsigned int i) const {
  return registeredCollections[i];
}

long unsigned int CollectionRegistry::operator()(const std::string& string) {
  std::lock_guard<std::mutex> lock(registryMutex);

  auto it = index.find(string);
  if ( it == index.end() ) {
    Values values; // TODO: parse values
    
    registeredCollections.emplace_back(string);
    index[string] = index.size();
    return registeredCollections.size()-1;
  }
  return it->second;
}

// Create global registry
CollectionRegistry collectionRegistry = CollectionRegistry();
