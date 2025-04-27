#ifndef BPMNOS_Execution_ConditionalEventObserver_H
#define BPMNOS_Execution_ConditionalEventObserver_H

#include "Observable.h"
#include "Observer.h"
#include "DataUpdate.h"
#include "execution/utility/src/auto_list.h"

namespace BPMNOS::Execution {

class SystemState;
class Token;

class ConditionalEventObserver : public Observer {
public:
  ConditionalEventObserver();
  void connect(SystemState* systemState);
  void notice(const Observable* observable) override;
protected:
  SystemState* systemState;
  void triggerConditionalEvent(const DataUpdate* dataUpdate, auto_list< std::weak_ptr<Token> >& waitingTokens);
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_ConditionalEventObserver_H
