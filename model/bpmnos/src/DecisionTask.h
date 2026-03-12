#ifndef BPMNOS_Model_DecisionTask_H
#define BPMNOS_Model_DecisionTask_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {

class ExtensionElements;

/**
 * @brief Class representing a task in which one or more choices have to be made.
 */
class DecisionTask : public BPMN::Task {
  friend class Model;
public:
  DecisionTask(XML::bpmn::tTask* task, BPMN::Scope* parent);

  template <typename DataType>
  std::vector<std::vector<number>> enumerateAlternatives(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;

private:
  static void determineAlternatives(
    std::vector<std::vector<number>>& alternatives,
    const ExtensionElements* extensionElements,
    BPMNOS::Values& status,
    BPMNOS::Values& data,
    BPMNOS::Values& globals,
    std::vector<number>& choices,
    size_t index
  );
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_DecisionTask_H
