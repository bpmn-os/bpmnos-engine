#ifndef BPMNOS_Model_StringRegistry_H
#define BPMNOS_Model_StringRegistry_H

#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>

namespace BPMNOS {

 /**
   * @brief Utility class for representing strings by numeric values.
   *
   * The StringRegistry class provides efficient access to the string by index
   * and retrieval of the index by string.
   */
  struct StringRegistry {
    /// Constructor adds "false" and "true" at indices 0 and 1.
    StringRegistry();

    /// Operator providing access to a registered string by index.
    std::string operator[](size_t i) const;
    /// Operator to register a string and return its index.
    size_t operator()(const std::string& string);
    size_t size() const;
    void clear();
  private:
    std::vector<std::string> registeredStrings;
    std::unordered_map<std::string, size_t> index;
    mutable std::shared_mutex registryMutex;
  public:
    // Prevent use of copy constructor and assignment operator as mutex is not copyable
    StringRegistry(const StringRegistry &) = delete;
    StringRegistry &operator=(const StringRegistry &) = delete;
  };


} // namespace BPMNOS

#endif // BPMNOS_Model_StringRegistry_H

// `stringRegistry` is a global variable
extern BPMNOS::StringRegistry stringRegistry; 
