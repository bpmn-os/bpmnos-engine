#include "StringRegistry.h"

using namespace BPMNOS;

std::string_view StringRegistry::operator[](long unsigned int i) {
  return registeredStrings[i];
}

long unsigned int StringRegistry::operator()(const std::string& string) {
  auto it = index.find(string);
  if ( index.find(string) == index.end() ) {
    registeredStrings.push_back(string);
    index[string] = index.size();
    return registeredStrings.size()-1;
  }
  return it->second;
}

// Create global registry
StringRegistry stringRegistry;
