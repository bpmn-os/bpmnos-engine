#include "CollectionRegistry.h"
#include "Keywords.h"
#include <strutil.h>
#include <cassert>

using namespace BPMNOS;

CollectionRegistry::CollectionRegistry() {
  // register empty collection with index 0
  (*this)(std::vector<double>());
}

const std::vector<double>&  CollectionRegistry::operator[](size_t i) const {
  return registeredCollections[i];
}

size_t CollectionRegistry::operator()(const std::vector<double>& collection) {
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

size_t CollectionRegistry::size() const {
  return registeredCollections.size();
}

void CollectionRegistry::clear() {
  registeredCollections.clear();
  index.clear();
  (*this)(std::vector<double>());
}

// Create global registry
CollectionRegistry collectionRegistry = CollectionRegistry();
