#ifndef BPMNOS_Execution_DataUpdate_H
#define BPMNOS_Execution_DataUpdate_H

#include <vector>
#include <cassert>
#include "Observable.h"
#include "model/bpmnos/src/extensionElements/Attribute.h" 

namespace BPMNOS::Execution {


struct DataUpdate : Observable {
  constexpr Type getObservableType() const override { return Type::DataUpdate; };
  DataUpdate(const std::vector<const BPMNOS::Model::Attribute*>& attributes) : instanceId(-1), attributes(attributes) {}
  DataUpdate(const BPMNOS::number instanceId, const std::vector<const BPMNOS::Model::Attribute*>& attributes) : instanceId(instanceId), attributes(attributes) { assert(instanceId >= 0); }
  const BPMNOS::number instanceId;
  const std::vector<const BPMNOS::Model::Attribute*>& attributes;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_DataUpdate_H

