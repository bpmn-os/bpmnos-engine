#ifndef BPMNOS_StringRegistry_H
#define BPMNOS_StringRegistry_H

#include <string>
#include <vector>
#include <unordered_map>

namespace BPMNOS {

 /**
   * @brief Utility class for representing strings by numeric values.
   *
   * The StringRegistry class provides efficient access to the string by index
   * and retrieval of the index by string.
   */
  struct StringRegistry {
    /// Operator to access a registered string by index
    std::string operator[](long unsigned int i);
    /// Operator to register a string and return its index
    long unsigned int operator()(const std::string& string);
  private:
    std::vector<std::string> registeredStrings;
    std::unordered_map<std::string, long unsigned int> index;
  };

  extern StringRegistry stringRegistry; // `stringRegistry` is a global variable

} // namespace BPMNOS

#endif // BPMNOS_StringRegistry_H
