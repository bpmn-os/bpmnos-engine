#ifndef BPMNOS_Execution_auto_associative_schedule_H
#define BPMNOS_Execution_auto_associative_schedule_H

#include <set>
#include <tuple>
#include <memory>
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

/**
 * @brief Custom container for managing an ordered set of std::weak_ptr<T> with associated data of type U and automatic removal of expired elements
 */
template <typename T, typename U>
class auto_associative_schedule {
public:
  struct iterator {
    typename std::set< std::tuple< BPMNOS::number, std::weak_ptr<T>, U > >::iterator current;
    auto_associative_schedule<T,U>* container;

    iterator(typename std::set< std::tuple< BPMNOS::number, std::weak_ptr<T>, U > >::iterator iter, auto_associative_schedule<T,U>* cont)
      : current(iter), container(cont) {
        skipExpired();
      }

    iterator& operator++() {
      ++current;
      skipExpired();
      return *this;
    }

    std::tuple< BPMNOS::number, std::weak_ptr<T>, U > operator*() const {
      return *current;
    }

    bool operator!=(const iterator& other) const {
      return current != other.current;
    }

  private:
    void skipExpired() {
      while (current != container->weak_pointer_associations.end() && std::get<1>(*current).expired()) {
        current = container->weak_pointer_associations.erase(current);
      }
    }
  };

  iterator begin() {
    return iterator(weak_pointer_associations.begin(), this);
  }

  iterator end() {
    return iterator(weak_pointer_associations.end(), this);
  }

  iterator cbegin() const {
    return iterator(weak_pointer_associations.begin(), const_cast<auto_associative_schedule<T,U>*>(this));
  }

  iterator cend() const {
    return iterator(weak_pointer_associations.end(), const_cast<auto_associative_schedule<T,U>*>(this));
  }

  iterator begin() const {
    return iterator(weak_pointer_associations.begin(), const_cast<auto_associative_schedule<T,U>*>(this));
  }

  iterator end() const {
    return iterator(weak_pointer_associations.end(), const_cast<auto_associative_schedule<T,U>*>(this));
  }

  void emplace(BPMNOS::number key, const std::weak_ptr<T>& item, U data) {
    weak_pointer_associations.emplace(key, item, data);
  }

  void remove(T* item) {
    auto it = begin();
    while (it != end()) {
      auto shared = std::get<1>(*it.current).lock();
      if (shared && shared.get() == item) {
        weak_pointer_associations.erase(it.current);
        return;
      }
      ++it;
    }
  }

  bool empty() const {
    return weak_pointer_associations.empty();
  }

private:
  struct comparator {
    bool operator()(const std::tuple<BPMNOS::number, std::weak_ptr<T>, U >& lhs, const std::tuple<BPMNOS::number, std::weak_ptr<T>, U >& rhs) const {
      // Compare based on the number component
      if ( std::get<0>(lhs) != std::get<0>(rhs) ) {
        return std::get<0>(lhs) < std::get<0>(rhs);
      }
      // If times are equal, compare the underlying shared_ptr objects
      auto leftShared = std::get<1>(lhs).lock();
      if ( !leftShared ) return false;
      auto rightShared = std::get<1>(rhs).lock();
      if ( !rightShared ) return true;
      // Compare shared_ptr objects, if both are valid
      return (leftShared.get() < rightShared.get());
    }
  };
  mutable std::set< std::tuple< BPMNOS::number, std::weak_ptr<T>, U >, comparator > weak_pointer_associations;
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_auto_associative_schedule_H

