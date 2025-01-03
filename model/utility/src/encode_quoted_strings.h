#ifndef BPMNOS_encode_quoted_strings_H
#define BPMNOS_encode_quoted_strings_H

#include <string>
#include <unordered_map>
#include <regex>
#include <stack>
#include <tuple>

#include "StringRegistry.h"

namespace BPMNOS {

// Function to replace all strings within double quotes
std::string encodeQuotedStrings(std::string text) {
  std::regex doubleQuoteRegex("\"([^\"]*)\""); // Match strings within double quotes
  std::stack< std::tuple<size_t, size_t, long unsigned int> > replacements;

  // collect all matches and their positions
  auto begin = std::sregex_iterator(text.begin(), text.end(), doubleQuoteRegex);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    std::string matchedString = (*it)[1]; // Extract content inside quotes
    auto id = stringRegistry(matchedString);
    replacements.emplace(it->position(), it->length(), id);
  }

  // replace matches in reverse order to preserve positions
  while (!replacements.empty()) {
    auto& [startPos, length, id] = replacements.top();
    text.replace(startPos, length, std::to_string(id));
    replacements.pop();
  }

  return text;
}

} // namespace BPMNOS
#endif // BPMNOS_encode_quoted_strings_H
