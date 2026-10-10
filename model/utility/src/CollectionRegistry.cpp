#include "CollectionRegistry.h"
#include "Keywords.h"
#include <cassert>
#include <stdexcept>

using namespace BPMNOS;

CollectionRegistry::~CollectionRegistry() {
  for ( auto& block : blocks ) {
    delete[] block.load(std::memory_order_relaxed);
  }
}

const CollectionRegistry::Entry& CollectionRegistry::entry(size_t i) const {
  // the entry is published before the counter is raised, so that an index below the counter refers to an
  // entry and a block that are completely written
  assert( i < published.load(std::memory_order_acquire) );
  return blocks[i / blockSize].load(std::memory_order_acquire)[i % blockSize];
}

const std::vector<double>& CollectionRegistry::operator[](size_t i) const {
  return entry(i).values;
}

ValueType CollectionRegistry::memberType(size_t i) const {
  return entry(i).memberType;
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
  if ( auto it = typeIndex.find(collection);
    it != typeIndex.end()
  ) {
    // registered by another thread in the meantime
    return it->second;
  }

  size_t i = published.load(std::memory_order_relaxed);
  if ( i / blockSize >= maxBlocks ) {
    throw std::runtime_error("CollectionRegistry: too many collections");
  }
  typeIndex.emplace(collection, i);
  Entry* block = blocks[i / blockSize].load(std::memory_order_relaxed);
  if ( !block ) {
    block = new Entry[blockSize];
    blocks[i / blockSize].store(block, std::memory_order_release);
  }
  block[i % blockSize] = Entry{collection, memberType};
  published.store(i + 1, std::memory_order_release);

  return i;
}

size_t CollectionRegistry::size() const {
  return published.load(std::memory_order_acquire);
}

// Create global registry
CollectionRegistry collectionRegistry = CollectionRegistry();
