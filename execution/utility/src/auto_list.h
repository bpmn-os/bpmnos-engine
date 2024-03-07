#ifndef BPMNOS_Execution_auto_list_H
#define BPMNOS_Execution_auto_list_H

#include <list>
#include <tuple>
#include <memory>
#include "expired.h"

namespace BPMNOS::Execution {

/**
 * @brief List of tuples with automatic removal of tuples containing an expired weak_ptr.
 */
template <typename... U>
class auto_list {
public:
  struct iterator {
    typename std::list< std::tuple< U... > >::iterator current;
    auto_list<U...>* container;

    iterator(typename std::list< std::tuple< U... > >::iterator iter, auto_list<U...>* cont)
      : current(iter), container(cont) {
        skipExpired();
      }

    iterator& operator++() {
      ++current;
      skipExpired();
      return *this;
    }

    std::tuple< U... >& operator*() const {
      return *current;
    }

    bool operator!=(const iterator& other) const {
      return current != other.current;
    }

  private:
    void skipExpired() {
      while (current != container->tuples.end() && expired(*current)) {
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
    return iterator(tuples.begin(), const_cast<auto_list<U...>*>(this));
  }

  iterator cend() const {
    return iterator(tuples.end(), const_cast<auto_list<U...>*>(this));
  }

  iterator begin() const {
    return iterator(tuples.begin(), const_cast<auto_list<U...>*>(this));
  }

  iterator end() const {
    return iterator(tuples.end(), const_cast<auto_list<U...>*>(this));
  }

  void emplace_back(U... data) {
    tuples.emplace_back(data...);
  }

  template <typename T>
  void remove(T* pointer) {
    tuples.remove_if([pointer](const std::tuple< U... >& elements) {
      auto& element = std::get< std::weak_ptr<T> >(elements);
      if ( auto shared = element.lock() ) {
        return shared.get() == pointer;
      }
      return false;
    });
  }  

  bool empty() const {
    for ( [[maybe_unused]] auto& _ : *this ) {
      return false;
    }
    return true;
  }

private:
  mutable std::list< std::tuple< U... > > tuples;
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_auto_list_H
