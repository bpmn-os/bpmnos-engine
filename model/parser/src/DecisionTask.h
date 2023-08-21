#ifndef BPMNOS_DecisionTask_H
#define BPMNOS_DecisionTask_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>
#include "model/utility/src/Number.h"

namespace BPMNOS {


class DecisionTask : public BPMN::Task {
  friend class Model;
public:
  DecisionTask(XML::bpmn::tTask* task, BPMN::Scope* parent) [[deprecated("Incomplete implementation")]];

  [[deprecated("Incomplete implementation")]]
  void apply(const Values& choices, Values& status) const;

protected:
};

} // namespace BPMNOS

#endif // BPMNOS_DecisionTask_H
