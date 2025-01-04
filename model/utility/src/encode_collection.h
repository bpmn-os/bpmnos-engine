#ifndef BPMNOS_encode_collection_H
#define BPMNOS_encode_collection_H

#include <string>
#include <regex>

#include "CollectionRegistry.h"

namespace BPMNOS {

// Function to replace a collection following the prefix
std::string encodeCollection(std::string text, const std::string prefix = "") {
  std::string pattern = prefix + "[\\s]*\\[(.*?)\\]";
  std::regex regular_expression(pattern); // Match prefix followed by "[" ... "]"
  std::smatch match;

  if (std::regex_search(text, match, regular_expression)) {
    std::string collection = "[" + match[1].str() +"]";

    // Convert collection to a number using the registry
    auto number = collectionRegistry(collection);

    // Replace the matched substring with the number
    size_t startPos = (size_t)match.position(1)-1;
    size_t length = (size_t)match.length(1)+2;
    text.replace(startPos, length, std::to_string(number));
  }
  return text;
}

} // namespace BPMNOS
#endif // BPMNOS_encode_collection_H
