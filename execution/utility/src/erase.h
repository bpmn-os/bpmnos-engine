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
    throw std::logic_error("erase_ptr: cannot find element to be removed");
  }
  container.erase(it);
}

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_erase_H

