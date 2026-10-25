#ifndef BPMNOS_Model_ObjectRegistry_H
#define BPMNOS_Model_ObjectRegistry_H

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include "Object.h"

namespace BPMNOS {

/**
 * @brief Registry of constant objects, by which a literal is represented as a number.
 *
 * Every literal, `[ ... ]` and `{ name := ... }` alike, is registered here as a constant object, whose layout
 * refers to its constant schema, which states the value types of the literal and leaves every array open. The index
 * under which it is registered is the number standing for it wherever a value is a number: in an expression, in a
 * lookup table, and in an attribute holding it. Equal constants are registered once, so that two of them have the
 * same index; two constants are equal if their layouts, schemas included, and their values are equal. The schemas
 * take part because a string and a number may be held as the same number. A constant is found among those
 * registered by a hash of its layout and its values, computed without converting a value, and a constant with an
 * equal hash is compared in full, so that a collision never merges different constants.
 *
 * A registered object never moves and never changes: the entries are held in blocks of fixed size, each
 * allocated once, and the number of entries published so far is held in an atomic counter, which a
 * registration raises only after writing its entry. A reference to a registered object therefore stays valid
 * however many objects are registered later, and reading an entry takes no lock; only a registration takes the
 * mutex, so that concurrent registrations of the same object yield one index.
 */
struct ObjectRegistry {
  ObjectRegistry() = default;
  ~ObjectRegistry();

  /// Returns the registered object with the given index, without taking a lock.
  const std::shared_ptr<const Object>& operator[](size_t i) const;
  /// Registers the object, unless an equal constant is registered already, and returns its index.
  size_t operator()(Object object);
  size_t size() const;

  ObjectRegistry(const ObjectRegistry&) = delete;
  ObjectRegistry& operator=(const ObjectRegistry&) = delete;
private:
  static constexpr size_t blockSize = 4096; ///< The number of entries per block
  static constexpr size_t maxBlocks = 16384; ///< The number of blocks, bounding the registry at 2^26 entries
  /// The blocks of entries, each allocated by the registration filling its first entry and never moved.
  std::array< std::atomic< std::shared_ptr<const Object>* >, maxBlocks > blocks{};
  /// The number of published entries, raised by a registration after its entry is written.
  std::atomic<size_t> published{0};
  /// The indices of the registered constants by the hash of their layout and values.
  std::unordered_multimap<size_t, size_t> index;
  /// Returns the index of a registered constant equal to the given one with the given hash, if there is one.
  std::optional<size_t> find(const Object& object, size_t hash) const;
  mutable std::shared_mutex registryMutex; ///< Guards the index and serialises registrations
};

} // namespace BPMNOS

#endif // BPMNOS_Model_ObjectRegistry_H

// `ObjectRegistry` is a global variable
extern BPMNOS::ObjectRegistry objectRegistry;
