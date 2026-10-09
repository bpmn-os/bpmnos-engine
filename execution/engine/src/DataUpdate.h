#ifndef BPMNOS_Execution_DataUpdate_H
#define BPMNOS_Execution_DataUpdate_H

#include <vector>
#include <cassert>
#include "Observable.h"
#include "model/bpmnos/src/extensionElements/Attribute.h"

namespace BPMNOS::Execution {


/**
 * @brief Notification that data attributes were written by a token of an instance.
 *
 * A written attribute concerns that instance alone, unless it is a global attribute, whose index is below the
 * instance index of the model and which concerns every instance; an observer decides this by the index of each
 * attribute.
 */
struct DataUpdate : Observable {
  constexpr Type getObservableType() const override { return Type::DataUpdate; };
  DataUpdate(const BPMNOS::number instanceId, const std::vector<const BPMNOS::Model::Attribute*>& attributes) : instanceId(instanceId), attributes(attributes) { assert(instanceId >= 0); }
  const BPMNOS::number instanceId;
  const std::vector<const BPMNOS::Model::Attribute*>& attributes;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DataUpdate_H

