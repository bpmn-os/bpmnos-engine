#include "VectorRegistry.h"
#include "model/utility/src/StringRegistry.h"
#include <nlohmann/json.hpp>

using namespace BPMNOS;

VectorRegistry::VectorRegistry() {
}

const Values& VectorRegistry::operator[](long unsigned int  stringIndex) {
  std::lock_guard<std::mutex> lock(registryMutex);

  auto it = registeredVectors.find(stringIndex);
  if ( it == registeredVectors.end() ) {
    // parse string and get values
    nlohmann::json jsonArray = nlohmann::json::parse( stringRegistry[stringIndex] );

    Values values;
    for (const auto& element : jsonArray) {
      if (element.is_string()) {
        // Element is a string
        values.push_back( BPMNOS::to_number( element.get<std::string>(), STRING ) );
      }
      else if (element.is_boolean()) {
        // Element is a boolean
        values.push_back( (int)element.get<bool>() );
      }
      else if (element.is_number()) {
        // Element is a number
        values.push_back( (float)element.get<double>() );
      }
      else if (element.is_null()) {
        // Element is a number
        values.push_back(std::nullopt);
      }
      else {
        // Element is of another type (i.e. object or array)
        throw std::runtime_error("VectorRegistry: failed parsing vector '" + stringRegistry[(long unsigned int)stringIndex] + "'");
      }
    }

    registeredVectors[stringIndex] = values;
    return registeredVectors[stringIndex];
  }
  return it->second;
}

// Create global registry
VectorRegistry vectorRegistry = VectorRegistry();
