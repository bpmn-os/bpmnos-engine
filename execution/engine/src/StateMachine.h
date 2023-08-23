#ifndef BPMNOS_StateMachine_H
#define BPMNOS_StateMachine_H

#include "Token.h"
#include "model/data/src/InstanceData.h"

namespace BPMNOS {

class StateMachine {
public:
  StateMachine(const InstanceData* instance);
  const InstanceData* instance;
private:
};

} // namespace BPMNOS

#endif // BPMNOS_StateMachine_H
