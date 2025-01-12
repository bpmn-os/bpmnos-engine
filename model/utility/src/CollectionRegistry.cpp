#include "CollectionRegistry.h"
#include "Keywords.h"
#include <strutil.h>

using namespace BPMNOS;

Collection::Collection(const std::string& collection)
  : collection(strutil::trim_copy(collection))
{
  // get values
  if ( !strutil::starts_with(collection,"[") || !strutil::ends_with(collection,"]") ) {
    throw std::runtime_error("Collection: string '" + collection + "' must start with '[' and must end with ']'");
  }
  // determine list of comma separated values
  auto list = collection.substr(1,collection.size()-2);
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
        throw std::runtime_error("Collection: illegal value '" + element + "' in '" + collection + "'");
      }
    }
  }
}

CollectionRegistry::CollectionRegistry() {
  // register empty collection with index 0
  (*this)("[]");
}

const Collection&  CollectionRegistry::operator[](size_t i) const {
  return registeredCollections[i];
}

size_t CollectionRegistry::operator()(const std::string& collection) {
  std::lock_guard<std::mutex> lock(registryMutex);

  auto it = index.find(collection);
  if ( it == index.end() ) {
//    Values values; // TODO: parse values    
    registeredCollections.emplace_back(collection);
    index[collection] = index.size();
    return registeredCollections.size()-1;
  }
  return it->second;
}

void CollectionRegistry::clear() {
  registeredCollections.clear();
  index.clear();
  (*this)("[]");
}

// Create global registry
CollectionRegistry collectionRegistry = CollectionRegistry();
