#include "ObjectRegistry.h"
#include <cassert>
#include <stdexcept>
#include <functional>

using namespace BPMNOS;

namespace {

/// Returns the hash of a constant: of its schema and its layout, which are short, and of its values, an undefined
/// value being distinguished from every number.
size_t computeHash(const Object& object, const Schema& schema) {
  size_t hash = std::hash<std::string>{}( schema.stringify() );
  auto combine = [&hash](size_t value) {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6) + (hash >> 2);
  };
  combine( std::hash<std::string>{}( object.layout->stringify() ) );
  for ( auto& value : object.values ) {
    combine( value.has_value() ? std::hash<double>{}( (double)value.value() ) : 0x5bd1e995ULL );
  }
  return hash;
}

} // namespace

ObjectRegistry::~ObjectRegistry() {
  for ( auto& block : blocks ) {
    delete[] block.load(std::memory_order_relaxed);
  }
}

const std::pair<std::shared_ptr<const Object>, Schema>& ObjectRegistry::getEntry(size_t i) const {
  // the entry is published before the counter is raised, so that an index below the counter refers to an
  // entry and a block that are completely written
  if ( i >= published.load(std::memory_order_acquire) ) {
    throw std::logic_error("ObjectRegistry: no object with index " + std::to_string(i));
  }
  return blocks[i / blockSize].load(std::memory_order_acquire)[i % blockSize];
}

const std::shared_ptr<const Object>& ObjectRegistry::operator[](size_t i) const {
  return getEntry(i).first;
}

const Schema& ObjectRegistry::schema(size_t i) const {
  return getEntry(i).second;
}

std::optional<size_t> ObjectRegistry::find(const Object& object, const Schema& schema, size_t hash) const {
  auto [first, last] = index.equal_range(hash);
  for ( auto it = first; it != last; ++it ) {
    auto& [candidate, candidateSchema] = blocks[it->second / blockSize].load(std::memory_order_acquire)[it->second % blockSize];
    if ( candidateSchema == schema && *candidate->layout == *object.layout && candidate->values == object.values ) {
      return it->second;
    }
  }
  return std::nullopt;
}

size_t ObjectRegistry::operator()(Object object, Schema schema) {
  auto hash = computeHash(object, schema);

  std::shared_lock read_lock(registryMutex);
  if ( auto found = find(object, schema, hash) ) {
    return found.value();
  }
  read_lock.unlock();

  std::unique_lock write_lock(registryMutex);
  if ( auto found = find(object, schema, hash) ) {
    // registered by another thread in the meantime
    return found.value();
  }

  size_t i = published.load(std::memory_order_relaxed);
  if ( i / blockSize >= maxBlocks ) {
    throw std::runtime_error("ObjectRegistry: too many objects");
  }
  auto block = blocks[i / blockSize].load(std::memory_order_relaxed);
  if ( !block ) {
    block = new std::pair<std::shared_ptr<const Object>, Schema>[blockSize];
    blocks[i / blockSize].store(block, std::memory_order_release);
  }
  block[i % blockSize] = { std::make_shared<const Object>(std::move(object)), std::move(schema) };
  index.emplace(hash, i);
  published.store(i + 1, std::memory_order_release);
  return i;
}

size_t ObjectRegistry::size() const {
  return published.load(std::memory_order_acquire);
}

// Create global registry
ObjectRegistry objectRegistry = ObjectRegistry();
