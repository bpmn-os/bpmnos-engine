#ifndef BPMNOS_Execution_DataUpdate_H
#define BPMNOS_Execution_DataUpdate_H

#include <vector>
#include "Observable.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"

namespace BPMNOS::Execution {


/**
 * @brief Notification that data attributes were written by a token of an instance, by the environment of the run, or
 * that the engine advanced the time.
 *
 * A written attribute concerns that instance alone, unless it is a global attribute, whose index is below the
 * instance index of the model and which concerns every instance; an observer decides this by the index of each
 * attribute. An update written by no instance names the instance 0, the index of `false` in the string registry,
 * which no instance identifier may be: the update of the time at a clock tick, and the update of global attributes
 * by the environment. An update of the data of an instance by the environment names the process instance, also
 * where the data belongs to a scope within it, such as an event subprocess.
 */
struct DataUpdate : Observable {
  constexpr Type getObservableType() const override { return Type::DataUpdate; };
  DataUpdate(const BPMNOS::number instanceId, const std::vector<const BPMNOS::Model::Attribute*>& attributes) : instanceId(instanceId), attributes(attributes) {}
  const BPMNOS::number instanceId; ///< The process instance whose data was written, or 0 for global attributes written by no instance
  const std::vector<const BPMNOS::Model::Attribute*>& attributes;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DataUpdate_H

