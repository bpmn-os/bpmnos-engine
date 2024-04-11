#ifndef BPMNOS_Execution_auto_set_H
#define BPMNOS_Execution_auto_set_H

#include <set>
#include <tuple>
#include <memory>
#include "expired.h"

namespace BPMNOS::Execution {

/**
 * @brief Set of tuples ordered in increasing order of the first component with automatic removal of tuples containing an expired weak_ptr.
 */
template <typename V, typename... U>
class auto_set {
public:
  struct iterator {
    typename std::set< std::tuple< V, U... > >::iterator current;
    auto_set<V,U...>* container;

    iterator(typename std::set< std::tuple< V, U... > >::iterator iter, auto_set<V,U...>* cont)
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
    return iterator(tuples.begin(), const_cast<auto_set<V,U...>*>(this));
  }

  iterator cend() const {
    return iterator(tuples.end(), const_cast<auto_set<V,U...>*>(this));
  }

  iterator begin() const {
    return iterator(tuples.begin(), const_cast<auto_set<V,U...>*>(this));
  }

  iterator end() const {
    return iterator(tuples.end(), const_cast<auto_set<V,U...>*>(this));
  }

  void emplace(V value, U... data) {
    tuples.emplace(value, data...);
  }

  iterator erase(iterator it) {
    return iterator(tuples.erase(it.current), this);
  }

  template <typename T>
  void remove(T* pointer) {
    auto it = begin();
    while (it != end()) {
      auto& element = std::get< std::weak_ptr<T> >(*it.current);
      if ( auto shared = element.lock() ) {
        if ( shared.get() == pointer ) {
          tuples.erase(it.current);
          return;
        }
      }
      ++it;
    }
  }

  bool empty() const {
    for ( [[maybe_unused]] auto _ : *this ) {
      return false;
    }
    return true;
  }

  void clear() {
    tuples.clear();
  }
private:
  struct comparator {
    bool operator()(const std::tuple<V, U... >& lhs, const std::tuple<V, U... >& rhs) const {
      // Compare based on the value of the first component
      if ( std::get<0>(lhs) != std::get<0>(rhs) ) {
        return std::get<0>(lhs) < std::get<0>(rhs);
      }
      // Compare pointers, if both have same value
      return (&lhs < &rhs);
    }
  };
  mutable std::set< std::tuple< V, U... >, comparator > tuples;
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_auto_set_H

