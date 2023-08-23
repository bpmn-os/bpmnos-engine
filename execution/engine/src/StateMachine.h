#ifndef BPMNOS_StateMachine_H
#define BPMNOS_StateMachine_H

#include "Token.h"
#include "model/data/src/InstanceData.h"

namespace BPMNOS::Execution {

class StateMachine {
public:
  StateMachine(const BPMNOS::Model::InstanceData* instance);
  const BPMNOS::Model::InstanceData* instance;
private:
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_StateMachine_H
