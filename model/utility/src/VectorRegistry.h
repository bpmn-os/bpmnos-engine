#ifndef BPMNOS_Model_VectorRegistry_H
#define BPMNOS_Model_VectorRegistry_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "model/utility/src/Number.h"

namespace BPMNOS {

 /**
   * @brief Utility class for representing vectors by numeric values.
   *
   * The VectorRegistry class provides efficient access to a vector by index
   * and retrieval of the index by string.
   */
  struct VectorRegistry {
    VectorRegistry();

    /// Operator providing access to a registered vector by string index.
    const Values& operator[](long unsigned int stringIndex);
  private:
    /// Map holding the vector index for each string index
    std::unordered_map< long unsigned int, Values > registeredVectors;
    std::mutex registryMutex;
  public:
    // Prevent use of copy constructor and assignment operator as mutex is not copyable
    VectorRegistry(const VectorRegistry &) = delete;
    VectorRegistry &operator=(const VectorRegistry &) = delete;
  };


} // namespace BPMNOS

#endif // BPMNOS_Model_VectorRegistry_H

// `vectorRegistry` is a global variable
extern BPMNOS::VectorRegistry vectorRegistry; 
