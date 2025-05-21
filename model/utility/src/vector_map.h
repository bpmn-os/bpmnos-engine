#ifndef BPMNOS_vector_map_H
#define BPMNOS_vector_map_H

#include <cinttypes>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

namespace BPMNOS {

/**
 * @brief Wrapper class around `std::unordered_map` for maps with vector keys.
 *
 * Example usage:
 * ```cpp
 *  BPMNOS::vector_map< std::vector<int>, double > my_map;
 *  my_map[{1, 2, 3}] = 42.0;
 *  my_map[{4, 5, 6}] = 84.0;
 *
 *  for (const auto& [key, value] : my_map) {
 *    std::cout << "[";
 *    for (const auto& elem : key) {
 *      std::cout << elem << ", ";
 *    }
 *    std::cout << "] -> " << value << std::endl;
 *  }
 * ```
 **/
template<typename Key, typename Value>
class vector_map {
private:
  /// hashing for vectors
  class vector_hash {
  public:
    size_t operator()(const Key& vec) const {
      size_t hash = 0;
      for (const auto& elem : vec) {
        hash ^= std::hash<typename Key::value_type>()(elem) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
      }
      return hash;
    }
  };

  std::unordered_map<Key, Value, vector_hash> map;

public:
   // Type aliases
   using iterator = typename std::unordered_map<Key, Value, vector_hash>::iterator;
   using const_iterator = typename std::unordered_map<Key, Value, vector_hash>::const_iterator;
   using size_type = typename std::unordered_map<Key, Value, vector_hash>::size_type;

   // Element access
   Value& operator[](const Key& key) {
     return map[key];
   }

   bool contains(const Key& key) const {
     return map.contains(key);
   }

   Value& at(const Key& key) {
     return map.at(key);
   }

   const Value& at(const Key& key) const {
     return map.at(key);
   }

   // Iterators
   iterator begin() noexcept {
     return map.begin();
   }

   const_iterator begin() const noexcept {
     return map.begin();
   }

   const_iterator cbegin() const noexcept {
     return map.cbegin();
   }

   iterator end() noexcept {
     return map.end();
   }

   const_iterator end() const noexcept {
     return map.end();
   }

   const_iterator cend() const noexcept {
     return map.cend();
   }

   // Capacity
   bool empty() const noexcept {
     return map.empty();
   }

   size_type size() const noexcept {
     return map.size();
   }

   // Modifiers
   void clear() noexcept {
     map.clear();
   }

   std::pair<iterator, bool> insert(const std::pair<Key, Value>& value) {
     return map.insert(value);
   }

   std::pair<iterator, bool> emplace(const Key& key, const Value& value) {
     return map.emplace(key,value);
   }

   std::pair<iterator, bool> try_emplace(const Key& key, const Value& value) {
     return map.try_emplace(key,value);
   }

   void erase(iterator pos) {
     map.erase(pos);
   }

   size_type erase(const Key& key) {
     return map.erase(key);
   }

   void swap(vector_map& other) noexcept {
     map.swap(other.map);
   }

   // Lookup
   size_type count(const Key& key) const {
     return map.count(key);
   }

   iterator find(const Key& key) {
     return map.find(key);
   }

   const_iterator find(const Key& key) const {
     return map.find(key);
   }
};

} // namespace BPMNOS

#endif // BPMNOS_vector_map_H

