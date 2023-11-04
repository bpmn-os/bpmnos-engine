#ifndef BPMNOS_Execution_erase_H
#define BPMNOS_Execution_erase_H

namespace BPMNOS::Execution {

/**
 * @brief Erase a specific element from a vector of elements.
 *
 * This function searches for a specified element pointer in a vector of elements
 * and removes the corresponding element from the vector.
 *
 * @tparam T The type of elements stored in the vector.
 * @param container The vector from which the element will be removed.
 * @param elementPtr The pointer to the element that needs to be removed.
 *
 * @throws std::logic_error if the specified element is not found in the vector.
 */

template<typename T>
void erase_ptr(std::vector<T>& container, const T* elementPtr) {
  auto it = std::find_if(container.begin(), container.end(),
    [elementPtr](const T& element)
    {
       return &element == elementPtr;
    }
  );

  if (it == container.end()) {
    throw std::logic_error("cannot find element to be removed");
  }
  container.erase(it);
}

/**
 * @brief Erase a specific element from a vector by replacing it with the last element.
 * 
 **/
template<typename T>
void erase(std::vector<T>& container, const T& element) {
  auto it = std::find(container.begin(), container.end(), element);
  if (it != container.end()) {
    std::iter_swap(it, container.end() - 1);
    container.erase(container.end() - 1);
  }
}

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_erase_H

