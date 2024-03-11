#ifndef BPMNOS_Execution_Observable_H
#define BPMNOS_Execution_Observable_H

namespace BPMNOS::Execution {

struct Observable {
  enum class Type { Token, SequentialPerformerUpdate, Message, ReadyEvent, EntryEvent, ChoiceEvent, CompletionEvent, ExitEvent, MessageDeliveryEvent, ClockTickEvent, ErrorEvent, TerminationEvent, Count };
  virtual constexpr Type getObservableType() const = 0;
  ~Observable() {};
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_Observable_H
