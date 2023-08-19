#include "StringRegistry.h"

using namespace BPMNOS;

StringRegistry::StringRegistry() {
  // register "false" with index 0 and "true" with index 1
  (*this)("false");
  (*this)("true");
}

std::string_view StringRegistry::operator[](long unsigned int i) const {
  return registeredStrings[i];
}

long unsigned int StringRegistry::operator()(const std::string& string) {
  std::lock_guard<std::mutex> lock(registryMutex);

  auto it = index.find(string);
  if ( it == index.end() ) {
    registeredStrings.push_back(string);
    index[string] = index.size();
    return registeredStrings.size()-1;
  }
  return it->second;
}

// Create global registry
StringRegistry stringRegistry = StringRegistry();
