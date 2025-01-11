#include "StringRegistry.h"
#include "Keywords.h"
#include <cassert>

using namespace BPMNOS;

StringRegistry::StringRegistry() {
  // register false with index 0 and true with index 1
  (*this)(Keyword::False);
  (*this)(Keyword::True);
}

std::string StringRegistry::operator[](size_t i) const {
  assert( i < registeredStrings.size() );
  return registeredStrings[i];
}

size_t StringRegistry::operator()(const std::string& string) {
  std::lock_guard<std::mutex> lock(registryMutex);

  auto it = index.find(string);
  if ( it == index.end() ) {
    registeredStrings.push_back(string);
    index[string] = index.size();
    return registeredStrings.size()-1;
  }
  return it->second;
}

void StringRegistry::clear() {
  registeredStrings.clear();
  index.clear();
  (*this)(Keyword::False);
  (*this)(Keyword::True);  
}

// Create global registry
StringRegistry stringRegistry = StringRegistry();
