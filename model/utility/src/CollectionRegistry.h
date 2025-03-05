#ifndef BPMNOS_Model_CollectionRegistry_H
#define BPMNOS_Model_CollectionRegistry_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "Number.h"

namespace BPMNOS {

  struct Collection : std::vector<double> {
    Collection(const std::string& collection);
    std::string collection;
//    Values values;
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
    const Collection& operator[](size_t i) const;
    /// Operator to register a collection by its string representation and return its index.
    size_t operator()(const std::string& collection);
    void clear();
  private:
    std::vector<Collection> registeredCollections;
    std::unordered_map<std::string, size_t> index;
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
