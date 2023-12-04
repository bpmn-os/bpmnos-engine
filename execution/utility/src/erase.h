#ifndef BPMNOS_Execution_erase_H
#define BPMNOS_Execution_erase_H

namespace BPMNOS::Execution {

/**
 * @brief Erase a specific element from a vector of unique pointers.
 *
 * This function searches for a specified element pointer in a vector of unique pointers
 * and removes the corresponding element from the vector.
 *
 * @tparam T The type of elements stored in the vector.
 * @param container The vector from which the element will be removed.
 * @param elementPtr The pointer to the element that needs to be removed.
 *
 * @throws std::logic_error if the specified element is not found in the vector.
 */
template<typename T>
void erase_ptr(std::vector<std::unique_ptr<T>>& container, const T* elementPtr) {
  auto it = std::find_if(container.begin(), container.end(),
    [elementPtr](const std::unique_ptr<T>& uniquePtr)
    {
       return uniquePtr.get() == elementPtr;
    }
  );

  if (it == container.end()) {
    throw std::logic_error("erase_ptr: cannot find unique pointer to be removed");
  }
  container.erase(it);
}

/**
 * @brief Erase a specific element from a vector of shared pointers.
 *
 * This function searches for a specified element pointer in a vector of shared pointers
 * and removes the corresponding element from the vector.
 *
 * @tparam T The type of elements stored in the vector.
 * @param container The vector from which the element will be removed.
 * @param elementPtr The pointer to the element that needs to be removed.
 *
 * @throws std::logic_error if the specified element is not found in the vector.
 */
template<typename T>
void erase_ptr(std::vector<std::shared_ptr<T>>& container, const T* elementPtr) {
  auto it = std::find_if(container.begin(), container.end(),
    [elementPtr](const std::shared_ptr<T>& sharedPtr)
    {
       return sharedPtr.get() == elementPtr;
    }
  );

  if (it == container.end()) {
    throw std::logic_error("erase_ptr: cannot find shared pointer to be removed");
  }
  container.erase(it);
}

/**
 * @brief Erase a specific element from a vector of pointers.
 *
 * This function searches for a specified element pointer in a vector of pointers
 * and removes the corresponding element from the vector.
 *
 * @tparam T The type of elements stored in the vector.
 * @param container The vector from which the element will be removed.
 * @param elementPtr The pointer to the element that needs to be removed.
 *
 * @throws std::logic_error if the specified element is not found in the vector.
 */
template<typename T>
void erase_ptr(std::vector<T*>& container, const T* elementPtr) {
  auto it = std::find_if(container.begin(), container.end(),
    [elementPtr](const T* ptr)
    {
       return ptr == elementPtr;
    }
  );

  if (it == container.end()) {
    throw std::logic_error("erase_ptr: cannot find pointer to be removed");
  }
  container.erase(it);
}

/**
 * @brief Erase a specific element from a set of key-value pairs where the value is a pointer to the element.
 *
 * This function searches for a specified element pointer in a set of key-value pairs
 * and removes the corresponding element from the set.
 *
 * @tparam T The type of elements stored in the set.
 * @param container The set from which the element will be removed.
 * @param elementPtr The pointer to the element that needs to be removed.
 *
 * @throws std::logic_error if the specified element is not found in the set.
 */
template<typename K, typename T, typename Comparator>
void erase_pair(std::set< std::pair<K,T*>, Comparator >& container, const T* elementPtr) {
  auto it = std::find_if(container.begin(), container.end(),
    [elementPtr](const std::pair<K,T*>& element)
    {
       return element.second == elementPtr;
    }
  );

  if (it == container.end()) {
    throw std::logic_error("erase_pair: cannot find element to be removed");
  }
  container.erase(it);
}

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_erase_H

