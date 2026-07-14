#ifndef BPMNOS_string_utility_H
#define BPMNOS_string_utility_H

#include <string>
#include <vector>
#include <cctype>
#include <algorithm>

namespace BPMNOS {

/// Removes leading whitespace (in-place).
inline void trim_left(std::string& str) {
  str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
    return !std::isspace(ch);
  }));
}

/// Removes trailing whitespace (in-place).
inline void trim_right(std::string& str) {
  str.erase(std::find_if(str.rbegin(), str.rend(), [](unsigned char ch) {
    return !std::isspace(ch);
  }).base(), str.end());
}

/// Removes leading and trailing whitespace (in-place).
inline void trim(std::string& str) {
  trim_left(str);
  trim_right(str);
}

/// Returns a copy of the input with leading and trailing whitespace removed.
inline std::string trim_copy(std::string str) {
  trim(str);
  return str;
}

/// Splits the input at each occurrence of the (multi-character) delimiter.
/// A trailing token is always emitted, so the result contains
/// (number of delimiters + 1) tokens (e.g. "a," -> {"a",""}, "" -> {""}).
inline std::vector<std::string> split(const std::string& str, const std::string& delim) {
  std::vector<std::string> tokens;
  size_t pos_start = 0, pos_end, delim_len = delim.length();
  while ((pos_end = str.find(delim, pos_start)) != std::string::npos) {
    tokens.push_back(str.substr(pos_start, pos_end - pos_start));
    pos_start = pos_end + delim_len;
  }
  tokens.push_back(str.substr(pos_start));
  return tokens;
}

/// Splits the input at each occurrence of the single-character delimiter.
inline std::vector<std::string> split(const std::string& str, const char delim) {
  return split(str, std::string(1, delim));
}

/// Splits the input at each character that appears in delims. Consecutive
/// delimiters yield empty tokens, and a trailing token is always emitted.
inline std::vector<std::string> split_any(const std::string& str, const std::string& delims) {
  std::vector<std::string> tokens;
  size_t pos_start = 0;
  for (size_t pos_end = 0; pos_end < str.length(); ++pos_end) {
    if (delims.find(str[pos_end]) != std::string::npos) {
      tokens.push_back(str.substr(pos_start, pos_end - pos_start));
      pos_start = pos_end + 1;
    }
  }
  tokens.push_back(str.substr(pos_start));
  return tokens;
}

/// Replaces (in-place) every occurrence of target with replacement.
/// Returns true if at least one occurrence was found.
inline bool replace_all(std::string& str, const std::string& target, const std::string& replacement) {
  if (target.empty()) {
    return false;
  }
  size_t start_pos = 0;
  bool found = false;
  while ((start_pos = str.find(target, start_pos)) != std::string::npos) {
    str.replace(start_pos, target.length(), replacement);
    start_pos += replacement.length();
    found = true;
  }
  return found;
}

} // namespace BPMNOS
#endif // BPMNOS_string_utility_H
