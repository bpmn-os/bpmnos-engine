#ifndef BPMNOS_Model_DecisionTask_H
#define BPMNOS_Model_DecisionTask_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {


/**
 * @brief Class representing a task is which one or more decisions have to be made.
 */
class DecisionTask : public BPMN::Task {
  friend class Model;
public:
  DecisionTask(XML::bpmn::tTask* task, BPMN::Scope* parent);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_DecisionTask_H
