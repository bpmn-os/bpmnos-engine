#ifndef BPMNOS_StringRegistry_H
#define BPMNOS_StringRegistry_H

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <mutex>

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

    /// Operator providing read-access to a registered string by index.
    std::string_view operator[](long unsigned int i) const;
    /// Operator to register a string and return its index.
    long unsigned int operator()(const std::string& string);
  private:
    std::vector<std::string> registeredStrings;
    std::unordered_map<std::string, long unsigned int> index;
    std::mutex registryMutex;
  public:
    // Prevent use of copy constructor and assignment operator as mutex is not copyable
    StringRegistry(const StringRegistry &) = delete;
    StringRegistry &operator=(const StringRegistry &) = delete;
  };

  extern StringRegistry stringRegistry; // `stringRegistry` is a global variable

} // namespace BPMNOS

#endif // BPMNOS_StringRegistry_H
