#ifndef BPMNOS_Model_CollectionRegistry_H
#define BPMNOS_Model_CollectionRegistry_H

#include <array>
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
   */
  struct CollectionRegistry {
    /// Constructor creating an empty registry. Declared because the deleted copy constructor below
    /// would otherwise suppress the implicit default constructor.
    CollectionRegistry() = default;

    /// Operator providing access to a registered collections by index.
    /// Returns by value: a reference would dangle if a concurrent operator() push_back reallocates
    /// registeredCollections after the read lock is released.
    std::vector<double> operator[](size_t i) const;
    /// Returns the type of the members of the registered collection with the given index.
    ValueType memberType(size_t i) const;
    /// Operator to register a collection by its values and the type of its members and return its index.
    size_t operator()(const std::vector<double>& collection, ValueType memberType);
    size_t size() const;
    void clear();
  private:
    std::vector< std::vector<double> > registeredCollections;
    std::vector< ValueType > registeredMemberTypes;
    /// One index per member type, so that collections holding the same numbers as members of different
    /// types are registered separately.
    std::array< vector_map<std::vector<double>, size_t>, COLLECTION + 1 > index;
    mutable std::shared_mutex registryMutex;
  public:
    // Prevent use of copy constructor and assignment operator as mutex is not copyable
    CollectionRegistry(const CollectionRegistry &) = delete;
    CollectionRegistry &operator=(const CollectionRegistry &) = delete;
  };


} // namespace BPMNOS

#endif // BPMNOS_Model_CollectionRegistry_H

// `CollectionRegistry` is a global variable
extern BPMNOS::CollectionRegistry collectionRegistry; 
