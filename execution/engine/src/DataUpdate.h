#ifndef BPMNOS_Execution_DataUpdate_H
#define BPMNOS_Execution_DataUpdate_H

#include <vector>
#include "Observable.h"
#include "model/bpmnos/src/extensionElements/Attribute.h" 

namespace BPMNOS::Execution {


struct DataUpdate : Observable {
  constexpr Type getObservableType() const override { return Type::DataUpdate; };
  DataUpdate(const BPMNOS::number instanceId, const std::vector<const BPMNOS::Model::Attribute*>& updates) : instanceId(instanceId), updates(updates) {}
  const BPMNOS::number instanceId;
  const std::vector<const BPMNOS::Model::Attribute*>& updates;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DataUpdate_H

