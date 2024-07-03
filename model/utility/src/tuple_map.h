#ifndef BPMNOS_tuple_map_H
#define BPMNOS_tuple_map_H

#include <cinttypes>
#include <cstddef>
#include <functional>
#include <unordered_map>
#include <tuple>

namespace BPMNOS {

/**
 * @brief Wrapper class around `std::unordered_map` for maps with tuple keys.
 *
 * Example usage:
 * ```cpp
 *  BPMNOS::tuple_map< std::tuple< int, std::string >, double > my_map;
 *  my_map[std::make_tuple(1, "example")] = 42.0;
 *  my_map[std::make_tuple(2, "test")] = 84.0;
 *
 *  for (const auto& [key, value] : my_map) {
 *    std::cout << "(" << std::get<0>(key) << ", " << std::get<1>(key) << ") -> " << value << std::endl;
 *  }
 * ```
 **/
template<typename Key, typename Value>
class tuple_map {
private:
  /// hashing for tuples
  class tuple_hash {
    template<class T>
    struct component {
      const T& value;
      component(const T& value) : value(value) {}
      uintmax_t operator,(uintmax_t n) const {
        n ^= std::hash<T>()(value);
        n ^= n << (sizeof(uintmax_t) * 4 - 1);
        return n ^ std::hash<uintmax_t>()(n);
      }
    };

  public:
    template<class Tuple>
    size_t operator()(const Tuple& tuple) const {
      return std::hash<uintmax_t>()( std::apply([](const auto& ... xs) { return (component(xs), ..., 0); }, tuple) );
    }
  };

  std::unordered_map<Key, Value, tuple_hash> map;
public:
   // Type aliases
   using iterator = typename std::unordered_map<Key, Value, tuple_hash>::iterator;
   using const_iterator = typename std::unordered_map<Key, Value, tuple_hash>::const_iterator;
   using size_type = typename std::unordered_map<Key, Value, tuple_hash>::size_type;

   // Element access
   Value& operator[](const Key& key) {
     return map[key];
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

   void erase(iterator pos) {
     map.erase(pos);
   }

   size_type erase(const Key& key) {
     return map.erase(key);
   }

   void swap(tuple_map& other) noexcept {
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
#endif // BPMNOS_tuple_map_H
