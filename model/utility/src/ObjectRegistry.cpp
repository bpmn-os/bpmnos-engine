#include "ObjectRegistry.h"
#include <cassert>
#include <stdexcept>

using namespace BPMNOS;

namespace {

/// Returns the key identifying an object: its layout and its values, an undefined value written as such.
std::string keyOf(const Object& object) {
  std::string key = object.layout->stringify() + "|";
  for ( auto& value : object.values ) {
    key += value.has_value() ? std::to_string((double)value.value()) : "_";
    key += ",";
  }
  return key;
}

} // namespace

ObjectRegistry::~ObjectRegistry() {
  for ( auto& block : blocks ) {
    delete[] block.load(std::memory_order_relaxed);
  }
}

const std::shared_ptr<const Object>& ObjectRegistry::operator[](size_t i) const {
  // the entry is published before the counter is raised, so that an index below the counter refers to an
  // entry and a block that are completely written
  if ( i >= published.load(std::memory_order_acquire) ) {
    throw std::logic_error("ObjectRegistry: no object with index " + std::to_string(i));
  }
  return blocks[i / blockSize].load(std::memory_order_acquire)[i % blockSize];
}

size_t ObjectRegistry::operator()(Object object) {
  auto key = keyOf(object);

  std::shared_lock read_lock(registryMutex);
  if ( auto it = index.find(key); it != index.end() ) {
    return it->second;
  }
  read_lock.unlock();

  std::unique_lock write_lock(registryMutex);
  if ( auto it = index.find(key); it != index.end() ) {
    // registered by another thread in the meantime
    return it->second;
  }

  size_t i = published.load(std::memory_order_relaxed);
  if ( i / blockSize >= maxBlocks ) {
    throw std::runtime_error("ObjectRegistry: too many objects");
  }
  auto block = blocks[i / blockSize].load(std::memory_order_relaxed);
  if ( !block ) {
    block = new std::shared_ptr<const Object>[blockSize];
    blocks[i / blockSize].store(block, std::memory_order_release);
  }
  block[i % blockSize] = std::make_shared<const Object>(std::move(object));
  index.emplace(std::move(key), i);
  published.store(i + 1, std::memory_order_release);
  return i;
}

size_t ObjectRegistry::size() const {
  return published.load(std::memory_order_acquire);
}

// Create global registry
ObjectRegistry objectRegistry = ObjectRegistry();
