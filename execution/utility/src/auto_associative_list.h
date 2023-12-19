#ifndef BPMNOS_Execution_auto_associative_list_H
#define BPMNOS_Execution_auto_associative_list_H

#include <list>
#include <memory>

namespace BPMNOS::Execution {

/**
 * @brief Custom container for managing an list of std::weak_ptr<T> with associated data of type U and automatic removal of expired elements.
 */
template <typename T, typename U>
class auto_associative_list {
public:
  struct iterator {
    typename std::list< std::pair< std::weak_ptr<T>, U > >::iterator current;
    auto_associative_list<T,U>* container;

    iterator(typename std::list< std::pair< std::weak_ptr<T>, U > >::iterator iter, auto_associative_list<T,U>* cont)
      : current(iter), container(cont) {
        skipExpired();
      }

    iterator& operator++() {
      ++current;
      skipExpired();
      return *this;
    }

    std::pair< std::weak_ptr<T>, U >& operator*() const {
      return *current;
    }

    bool operator!=(const iterator& other) const {
      return current != other.current;
    }

  private:
    void skipExpired() {
      while (current != container->weak_pointer_associations.end() && current->first.expired()) {
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
    return iterator(weak_pointer_associations.begin(), const_cast<auto_associative_list<T,U>*>(this));
  }

  iterator cend() const {
    return iterator(weak_pointer_associations.end(), const_cast<auto_associative_list<T,U>*>(this));
  }

  iterator begin() const {
    return iterator(weak_pointer_associations.begin(), const_cast<auto_associative_list<T,U>*>(this));
  }

  iterator end() const {
    return iterator(weak_pointer_associations.end(), const_cast<auto_associative_list<T,U>*>(this));
  }

  void emplace_back(const std::weak_ptr<T>& item, U data) {
    weak_pointer_associations.emplace_back(item,data);
  }

  void remove(T* item) {
    weak_pointer_associations.remove_if([item](const std::pair< std::weak_ptr<T>, U >& wp) {
      auto shared = wp.first.lock();
      return shared && shared.get() == item;
    });
  }  

  bool empty() const {
    return weak_pointer_associations.empty();
  }

private:
  mutable std::list< std::pair< std::weak_ptr<T>, U > > weak_pointer_associations;
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_auto_associative_list_H
