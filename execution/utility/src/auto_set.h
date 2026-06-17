#ifndef BPMNOS_Execution_auto_set_H
#define BPMNOS_Execution_auto_set_H

#include <algorithm>
#include <set>
#include <tuple>
#include <memory>
#include <concepts>
#include "expired.h"

namespace BPMNOS::Execution {

/**
 * @brief Orders tuples by increasing value of the first component (ties broken by address).
 */
struct ascending {
  template <typename Tuple>
  bool operator()(const Tuple& lhs, const Tuple& rhs) const {
    if ( std::get<0>(lhs) != std::get<0>(rhs) ) {
      return std::get<0>(lhs) < std::get<0>(rhs);
    }
    return (&lhs < &rhs); // distinct tuples with equal first component coexist, ordered by address
  }
};

/**
 * @brief Orders tuples by decreasing value of the first component (ties broken by address).
 */
struct descending {
  template <typename Tuple>
  bool operator()(const Tuple& lhs, const Tuple& rhs) const {
    if ( std::get<0>(lhs) != std::get<0>(rhs) ) {
      return std::get<0>(lhs) > std::get<0>(rhs);
    }
    return (&lhs < &rhs); // distinct tuples with equal first component coexist, ordered by address
  }
};

/// Satisfied by a type that can order two tuples of the given type (data element types like weak_ptr are not).
template <typename C, typename Tuple>
concept TupleComparator = requires (const C& c, const Tuple& a, const Tuple& b) {
  { c(a, b) } -> std::convertible_to<bool>;
};

/**
 * @brief Set of tuples ordered by the first component, with automatic removal of tuples containing an expired weak_ptr.
 *
 * Templated on the value type `V` of the first component and the trailing tuple element types `U...`. Pass a
 * comparator as the second template argument to choose the order — `auto_set<V, descending, U...>`; with no
 * comparator (`auto_set<V, U...>`) it orders ascending. The two forms are distinguished by the
 * `TupleComparator` concept, so a data type in the second position is never mistaken for a comparator.
 */
template <typename V, typename... U>
class auto_set;

// Comparator given explicitly as the second argument.
template <typename V, typename Compare, typename... U>
  requires TupleComparator< Compare, std::tuple<V, U...> >
class auto_set<V, Compare, U...> {
public:
  struct iterator {
    typename std::set< std::tuple< V, U... >, Compare >::iterator current;
    auto_set<V, Compare, U...>* container;

    iterator(typename std::set< std::tuple< V, U... >, Compare >::iterator iter, auto_set<V, Compare, U...>* cont)
      : current(iter), container(cont) {
        skipExpired();
      }

    iterator& operator++() {
      ++current;
      skipExpired();
      return *this;
    }

    std::tuple< V, U... > operator*() const {
      return *current;
    }

    bool operator!=(const iterator& other) const {
      return current != other.current;
    }

  private:
    void skipExpired() {
      // erase all tuples in which any of the initial weak_ptrs are expired
      while ( current != container->tuples.end() && expired<1>(*current) ) {
        current = container->tuples.erase(current);
      }
    }
  };

  iterator begin() {
    return iterator(tuples.begin(), this);
  }

  iterator end() {
    return iterator(tuples.end(), this);
  }

  iterator cbegin() const {
    return iterator(tuples.begin(), const_cast<auto_set<V,Compare,U...>*>(this));
  }

  iterator cend() const {
    return iterator(tuples.end(), const_cast<auto_set<V,Compare,U...>*>(this));
  }

  iterator begin() const {
    return iterator(tuples.begin(), const_cast<auto_set<V,Compare,U...>*>(this));
  }

  iterator end() const {
    return iterator(tuples.end(), const_cast<auto_set<V,Compare,U...>*>(this));
  }

  void emplace(V value, U... data) {
    tuples.emplace(value, data...);
  }

  iterator erase(iterator it) {
    return iterator(tuples.erase(it.current), this);
  }

  template <typename T>
  void remove(T* pointer) {
    if (auto it = find(pointer); it != end()) {
      tuples.erase(it.current);
    }
  }

  bool empty() const {
    for ( [[maybe_unused]] auto _ : *this ) {
      return false;
    }
    return true;
  }

  /**
   * @brief Counts active (non-expired) elements. O(n) complexity.
   */
  size_t count() const {
    size_t n = 0;
    for ( [[maybe_unused]] auto _ : *this ) { n++; }
    return n;
  }

  /**
   * @brief Finds an element by raw pointer. O(n) complexity.
   */
  template <typename T>
  iterator find(T* pointer) {
    auto it = std::find_if(tuples.begin(), tuples.end(),
      [pointer](const std::tuple<V, U...>& elements) {
        auto& element = std::get<std::weak_ptr<T>>(elements);
        if (auto shared = element.lock()) {
          return shared.get() == pointer;
        }
        return false;
      });
    return iterator(it, this);
  }

  /**
   * @brief Finds an element by raw pointer (const version). O(n) complexity.
   */
  template <typename T>
  iterator find(T* pointer) const {
    auto it = std::find_if(tuples.begin(), tuples.end(),
      [pointer](const std::tuple<V, U...>& elements) {
        auto& element = std::get<std::weak_ptr<T>>(elements);
        if (auto shared = element.lock()) {
          return shared.get() == pointer;
        }
        return false;
      });
    return iterator(it, const_cast<auto_set<V, Compare, U...>*>(this));
  }

  void clear() {
    tuples.clear();
  }

private:
  mutable std::set< std::tuple< V, U... >, Compare > tuples;
};

// No comparator given: order ascending by the first component.
template <typename V, typename... U>
class auto_set : public auto_set<V, ascending, U...> {};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_auto_set_H
