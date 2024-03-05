#ifndef BPMNOS_Model_CollectionRegistry_H
#define BPMNOS_Model_CollectionRegistry_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "Number.h"

namespace BPMNOS {

  struct Collection {
    Collection(const std::string& string);
    std::string string;
    Values values;
  };

 /**
   * @brief Utility class for representing collections by numeric values.
   *
   * The CollectionRegistry class provides efficient access to the collection by index
   * and retrieval of the index by the string representing the collection.
   */
  struct CollectionRegistry {
    /// Constructor adds empty collection at index 0.
    CollectionRegistry();

    /// Operator providing access to a registered collections by index.
    const Collection& operator[](long unsigned int i) const;
    /// Operator to register a collection by its string representation and return its index.
    long unsigned int operator()(const std::string& string);
  private:
    std::vector<Collection> registeredCollections;
    std::unordered_map<std::string, long unsigned int> index;
    std::mutex registryMutex;
  public:
    // Prevent use of copy constructor and assignment operator as mutex is not copyable
    CollectionRegistry(const CollectionRegistry &) = delete;
    CollectionRegistry &operator=(const CollectionRegistry &) = delete;
  };


} // namespace BPMNOS

#endif // BPMNOS_Model_CollectionRegistry_H

// `CollectionRegistry` is a global variable
extern BPMNOS::CollectionRegistry collectionRegistry; 