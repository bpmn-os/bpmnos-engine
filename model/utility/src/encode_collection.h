#ifndef BPMNOS_encode_collection_H
#define BPMNOS_encode_collection_H

#include <string>
#include <regex>
#include <cassert>
#include <strutil.h>

#include "CollectionRegistry.h"
#include "Keywords.h"

namespace BPMNOS {

/// Function to replace a collection that is not preceded by an alphanumeric or underscore
inline std::string encodeCollection(std::string text) {
  std::string pattern = "(^|[^a-zA-Z0-9_])\\[(.*?)\\]";// "[\\s]*\\[(.*?)\\]";
  std::regex regular_expression(pattern); // Match "[" ... "]" that is not preceded by an alphanumeric or underscore
  std::smatch match;

  while (std::regex_search(text, match, regular_expression)) {
    if ( match[2].str().contains("[") ) {
      throw std::runtime_error("Nested collections are not supported");
    }
    std::vector<double> collection;
    for ( auto value : strutil::split(match[2].str(),',') ) {
      strutil::trim(value);
      if ( value == Keyword::False ) {
        collection.push_back( false );
      }
      else if ( value == Keyword::True ) {
        collection.push_back( true );
      }
      else {
        try {
          // try to convert to number
          collection.push_back( std::stod(value) );
        }
        catch(...) {
          throw std::runtime_error("BPMNOS: illegal value '" + value + "' in '" + text + "'");
        }
      }
    }
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
