#ifndef BPMNOS_Model_CollectionRegistry_H
#define BPMNOS_Model_CollectionRegistry_H

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <shared_mutex>
#include "Number.h"
#include "Value.h"
#include "vector_map.h"

namespace BPMNOS {

 /**
   * @brief Utility class for representing collections by numeric values.
   *
   * The CollectionRegistry class provides efficient access to the collection by index
   * and retrieval of the index by the values representing the collection.
   *
   * A collection is registered together with the type of its members, which is the only record of what
   * its values mean: a member of a collection of strings is an index into the string registry, a member
   * of a collection of collections is an index into this registry, and neither can be told from a number
   * by inspection. A collection is therefore identified by its values and its member type together, so
   * that a collection of the numbers one and two and a collection of the two strings registered under
   * those indices remain distinct. Every collection has a member type, the empty collection included,
   * which is why the registry holds no collection until one is registered.
   *
   * A registered collection never moves and never changes: the entries are held in blocks of fixed size,
   * each allocated once, and the number of entries published so far is held in an atomic counter, which a
   * registration raises only after writing its entry. A reference to a registered collection therefore stays
   * valid however many collections are registered later, and reading an entry takes no lock; only a
   * registration takes the mutex, so that concurrent registrations of the same collection yield one index.
   */
  struct CollectionRegistry {
    /// Constructor creating an empty registry. Declared because the deleted copy constructor below
    /// would otherwise suppress the implicit default constructor.
    CollectionRegistry() = default;

    ~CollectionRegistry();

    /// Operator providing access to a registered collection by index, without taking a lock. The reference
    /// stays valid for the lifetime of the registry.
    const std::vector<double>& operator[](size_t i) const;
    /// Returns the type of the members of the registered collection with the given index.
    ValueType memberType(size_t i) const;
    /// Operator to register a collection by its values and the type of its members and return its index.
    size_t operator()(const std::vector<double>& collection, ValueType memberType);
    size_t size() const;
  private:
    struct Entry {
      std::vector<double> values;
      ValueType memberType = ValueType::DECIMAL;
    };
    static constexpr size_t blockSize = 4096; ///< The number of entries per block
    static constexpr size_t maxBlocks = 16384; ///< The number of blocks, bounding the registry at 2^26 entries
    /// The blocks of entries, each allocated by the registration filling its first entry and never moved.
    std::array< std::atomic<Entry*>, maxBlocks > blocks{};
    /// The number of published entries, raised by a registration after its entry is written.
    std::atomic<size_t> published{0};
    const Entry& entry(size_t i) const;
    /// One index per member type, so that collections holding the same numbers as members of different
    /// types are registered separately.
    std::array< vector_map<std::vector<double>, size_t>, COLLECTION + 1 > index;
    mutable std::shared_mutex registryMutex; ///< Guards the index and serialises registrations
  public:
    // Prevent use of copy constructor and assignment operator as mutex is not copyable
    CollectionRegistry(const CollectionRegistry &) = delete;
    CollectionRegistry &operator=(const CollectionRegistry &) = delete;
  };


} // namespace BPMNOS

#endif // BPMNOS_Model_CollectionRegistry_H

// `CollectionRegistry` is a global variable
extern BPMNOS::CollectionRegistry collectionRegistry; 
