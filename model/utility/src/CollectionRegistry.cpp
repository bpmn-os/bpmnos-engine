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
  assert( i < registeredCollections.size() );
  std::shared_lock read_lock(registryMutex);
  return registeredCollections[i];
}

size_t CollectionRegistry::operator()(const std::vector<double>& collection) {
  std::shared_lock read_lock(registryMutex);
  if ( auto it = index.find(collection);
    it != index.end()
  ) {
    return it->second;
  }
  read_lock.unlock();  

  std::unique_lock write_lock(registryMutex);
  auto [it, inserted] = index.try_emplace(collection, index.size());

  if ( !inserted ) {
    assert( index.size() == registeredCollections.size() );
    return it->second;
  }

  registeredCollections.push_back(collection);

  assert( index.size() == registeredCollections.size() );
  return registeredCollections.size()-1;
}

size_t CollectionRegistry::size() const {
  std::shared_lock read_lock(registryMutex);
  return registeredCollections.size();
}

void CollectionRegistry::clear() {
  std::unique_lock write_lock(registryMutex);
  index.clear();
  registeredCollections.clear();

  index.emplace(std::vector<double>(),0);  
  registeredCollections.push_back(std::vector<double>());
  
  assert( index.size() == registeredCollections.size() );
}

// Create global registry
CollectionRegistry collectionRegistry = CollectionRegistry();
