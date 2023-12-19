#ifndef BPMNOS_Execution_auto_schedule_H
#define BPMNOS_Execution_auto_schedule_H

#include <set>
#include <memory>
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

/**
 * @brief Custom container for managing an ordered set of std::weak_ptr<T> with automatic removal of expired elements.
 */
template <typename T>
class auto_schedule {
public:
  struct iterator {
    typename std::set< std::pair< BPMNOS::number, std::weak_ptr<T> > >::iterator current;
    auto_schedule<T>* container;

    iterator(typename std::set<std::pair<BPMNOS::number, std::weak_ptr<T>>>::iterator iter, auto_schedule<T>* cont)
      : current(iter), container(cont) {
        skipExpired();
      }

    iterator& operator++() {
      ++current;
      skipExpired();
      return *this;
    }

    std::pair< BPMNOS::number, std::weak_ptr<T> > operator*() const {
      return *current;
    }

    bool operator!=(const iterator& other) const {
      return current != other.current;
    }

  private:
    void skipExpired() {
      while (current != container->weak_pointers.end() && current->second.expired()) {
        current = container->weak_pointers.erase(current);
      }
    }
  };

  iterator begin() {
    return iterator(weak_pointers.begin(), this);
  }

  iterator end() {
    return iterator(weak_pointers.end(), this);
  }

  iterator cbegin() const {
    return iterator(weak_pointers.begin(), const_cast<auto_schedule<T>*>(this));
  }

  iterator cend() const {
    return iterator(weak_pointers.end(), const_cast<auto_schedule<T>*>(this));
  }

  iterator begin() const {
    return iterator(weak_pointers.begin(), const_cast<auto_schedule<T>*>(this));
  }

  iterator end() const {
    return iterator(weak_pointers.end(), const_cast<auto_schedule<T>*>(this));
  }

  void emplace(BPMNOS::number key, const std::weak_ptr<T>& item) {
    weak_pointers.emplace(key, item);
  }

  void remove(T* item) {
    auto it = begin();
    while (it != end()) {
      auto shared = it.current->second.lock();
      if (shared && shared.get() == item) {
        weak_pointers.erase(it.current);
        return;
      }
      ++it;
    }
  }

  bool empty() const {
    return weak_pointers.empty();
  }

private:
  struct comparator {
    bool operator()(const std::pair<BPMNOS::number, std::weak_ptr<T>>& lhs, const std::pair<BPMNOS::number, std::weak_ptr<T>>& rhs) const {
      // Compare based on the number component
      if (lhs.first != rhs.first) {
        return lhs.first < rhs.first;
      }
      // If times are equal, compare the underlying shared_ptr objects
      auto leftShared = lhs.second.lock();
      if ( !leftShared ) return false;
      auto rightShared = rhs.second.lock();
      if ( !rightShared ) return true;
      // Compare shared_ptr objects, if both are valid
      return (leftShared.get() < rightShared.get());
    }
  };
  mutable std::set< std::pair< BPMNOS::number, std::weak_ptr<T> >, comparator > weak_pointers;
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_auto_schedule_H

