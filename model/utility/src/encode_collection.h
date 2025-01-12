#ifndef BPMNOS_encode_collection_H
#define BPMNOS_encode_collection_H

#include <string>
#include <regex>
#include <cassert>
#include <iostream>

#include "CollectionRegistry.h"

namespace BPMNOS {

/// Function to replace a collection that is not preceded by an alphanumeric or underscore
inline std::string encodeCollection(std::string text) {
  std::string pattern = "(^|[^a-zA-Z0-9_])\\[(.*?)\\]";// "[\\s]*\\[(.*?)\\]";
  std::regex regular_expression(pattern); // Match "[" ... "]" that is not preceded by an alphanumeric or underscore
  std::smatch match;

  while (std::regex_search(text, match, regular_expression)) {
/*
std::cerr << match[0].str() << std::endl;  
std::cerr << match[1].str() << std::endl;  
std::cerr << match[2].str() << std::endl;  
    assert(match[1].str().front() == '[' && match[1].str().back() == ']');
*/
    if ( match[2].str().contains("[") ) {
      throw std::runtime_error("Nested collections are not supported");
    }
    std::string collection = "[" + match[2].str() + "]";
    // Convert collection to a number using the registry
    auto id = collectionRegistry(collection);

    // Replace the matched substring with the number
    size_t startPos = (size_t)match.position(2)-1;
    size_t length = (size_t)match.length(2)+2;
    text.replace(startPos, length, std::to_string(id));
  }
  return text;
}

} // namespace BPMNOS
#endif // BPMNOS_encode_collection_H
