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
  std::shared_lock read_lock(registryMutex);
  return registeredStrings[i];
}

size_t StringRegistry::operator()(const std::string& string) {
  std::shared_lock read_lock(registryMutex);
  if ( auto it = index.find(string);
    it != index.end()
  ) {
    return it->second;
  }
  read_lock.unlock();  

  std::unique_lock write_lock(registryMutex);
  auto [it, inserted] = index.try_emplace(string, index.size());

  if ( !inserted ) {
    assert( index.size() == registeredStrings.size() );
    return it->second;
  }

  registeredStrings.push_back(string);
  assert( index.size() == registeredStrings.size() );
  return registeredStrings.size()-1;
}

size_t StringRegistry::size() const {
  std::shared_lock read_lock(registryMutex);
  return registeredStrings.size();
}

void StringRegistry::clear() {
  std::unique_lock write_lock(registryMutex);

  index.clear();
  registeredStrings.clear();
  
  index.emplace(Keyword::False,0);  
  registeredStrings.push_back(Keyword::False);

  index.emplace(Keyword::True,1); 
  registeredStrings.push_back(Keyword::True);
  
  assert( index.size() == registeredStrings.size() );
}

// Create global registry
StringRegistry stringRegistry = StringRegistry();
