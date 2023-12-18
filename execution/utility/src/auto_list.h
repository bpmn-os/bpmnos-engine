#ifndef BPMNOS_Execution_auto_list_H
#define BPMNOS_Execution_auto_list_H

#include <list>
#include <memory>

namespace BPMNOS::Execution {

/**
 * @brief Custom container for managing a list of std::weak_ptr<T> with automatic removal of expired elements.
 */
template <typename T>
class auto_list {
public:
  struct iterator {
    typename std::list<std::weak_ptr<T>>::iterator current;
    auto_list<T>* container;

    iterator(typename std::list<std::weak_ptr<T>>::iterator iter, auto_list<T>* cont)
      : current(iter), container(cont) {
        skipExpired();
      }

    iterator& operator++() {
      ++current;
      skipExpired();
      return *this;
    }

    std::weak_ptr<T>& operator*() const {
      return *current;
    }

    bool operator!=(const iterator& other) const {
      return current != other.current;
    }

  private:
    void skipExpired() {
      while (current != container->weak_pointers.end() && current->expired()) {
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
    return iterator(weak_pointers.begin(), const_cast<auto_list<T>*>(this));
  }

  iterator cend() const {
    return iterator(weak_pointers.end(), const_cast<auto_list<T>*>(this));
  }

  iterator begin() const {
    return iterator(weak_pointers.begin(), const_cast<auto_list<T>*>(this));
  }

  iterator end() const {
    return iterator(weak_pointers.end(), const_cast<auto_list<T>*>(this));
  }

  void push_back(const std::weak_ptr<T>& item) {
    weak_pointers.push_back(item);
  }

  void remove(T* item) {
    weak_pointers.remove_if([item](const std::weak_ptr<T>& wp) {
      auto shared = wp.lock();
      return shared && shared.get() == item;
    });
  }  

  bool empty() const {
    return weak_pointers.empty();
  }

private:
  std::list<std::weak_ptr<T>> weak_pointers;
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_auto_list_H
