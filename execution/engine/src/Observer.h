#ifndef BPMNOS_Execution_Observer_H
#define BPMNOS_Execution_Observer_H

#include "Observable.h"

namespace BPMNOS::Execution {


class Observer {
public:
  virtual void notice(const Observable* observable) = 0;
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_Observer_H
