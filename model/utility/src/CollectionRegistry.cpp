#include "CollectionRegistry.h"
#include "Keywords.h"
#include <cassert>

using namespace BPMNOS;

std::vector<double> CollectionRegistry::operator[](size_t i) const {
  std::shared_lock read_lock(registryMutex);
  assert( i < registeredCollections.size() );
  return registeredCollections[i];
}

ValueType CollectionRegistry::memberType(size_t i) const {
  std::shared_lock read_lock(registryMutex);
  assert( i < registeredMemberTypes.size() );
  return registeredMemberTypes[i];
}

size_t CollectionRegistry::operator()(const std::vector<double>& collection, ValueType memberType) {
  auto& typeIndex = index[(size_t)memberType];

  std::shared_lock read_lock(registryMutex);
  if ( auto it = typeIndex.find(collection);
    it != typeIndex.end()
  ) {
    return it->second;
  }
  read_lock.unlock();

  std::unique_lock write_lock(registryMutex);
  auto [it, inserted] = typeIndex.try_emplace(collection, registeredCollections.size());

  if ( !inserted ) {
    assert( registeredMemberTypes.size() == registeredCollections.size() );
    return it->second;
  }

  registeredCollections.push_back(collection);
  registeredMemberTypes.push_back(memberType);

  assert( registeredMemberTypes.size() == registeredCollections.size() );
  return registeredCollections.size()-1;
}

size_t CollectionRegistry::size() const {
  std::shared_lock read_lock(registryMutex);
  return registeredCollections.size();
}

void CollectionRegistry::clear() {
  std::unique_lock write_lock(registryMutex);
  for ( auto& typeIndex : index ) {
    typeIndex.clear();
  }
  registeredCollections.clear();
  registeredMemberTypes.clear();
}

// Create global registry
CollectionRegistry collectionRegistry = CollectionRegistry();
