#ifndef BPMNOS_Execution_DataUpdate_H
#define BPMNOS_Execution_DataUpdate_H

#include <vector>
#include "Observable.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"

namespace BPMNOS::Execution {


/**
 * @brief Notification that data attributes were written by a token of an instance, or that the engine advanced the
 * time.
 *
 * A written attribute concerns that instance alone, unless it is a global attribute, whose index is below the
 * instance index of the model and which concerns every instance; an observer decides this by the index of each
 * attribute. The update of the time at a clock tick is written by no instance and names the instance 0, the index of
 * `false` in the string registry, which no instance identifier may be.
 */
struct DataUpdate : Observable {
  constexpr Type getObservableType() const override { return Type::DataUpdate; };
  DataUpdate(const BPMNOS::number instanceId, const std::vector<const BPMNOS::Model::Attribute*>& attributes) : instanceId(instanceId), attributes(attributes) {}
  const BPMNOS::number instanceId; ///< The instance whose token wrote the attributes, or 0 if the engine advanced the time
  const std::vector<const BPMNOS::Model::Attribute*>& attributes;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DataUpdate_H

