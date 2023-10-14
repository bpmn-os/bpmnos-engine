#ifndef BPMNOS_Token_H
#define BPMNOS_Token_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"

namespace BPMNOS::Execution {

class StateMachine;
class Token;
typedef std::vector< Token > Tokens;

class Event;
class EntryEvent;
class ExitEvent;
class ChoiceEvent;
class CompletionEvent;
class TriggerEvent;
class MessageDeliveryEvent;


class Token {
public:
  Token(const StateMachine* owner, const Values& status);
  const StateMachine* owner;
  const BPMN::FlowNode* node; 
  Values status;
  void run();
  void processEvent(const Event* event);

  bool ready() const { return state == State::READY; };
  bool entered() const { return state == State::ENTERED; };
  bool busy() const { return state == State::BUSY; };
  bool completed() const { return state == State::COMPLETED; };
  bool departed() const { return state == State::DEPARTED; };
  bool arrived() const { return state == State::ARRIVED; };
  bool done() const { return state == State::DONE; };
  bool failed() const { return state == State::FAILED; };
private:
  friend class StateMachine;
  friend class Engine;
  enum class State { CREATED, READY, ENTERED, BUSY, COMPLETED, DEPARTED, ARRIVED, DONE, FAILED, TO_BE_MERGED, TO_BE_COPIED };
  State state;
  bool advanceFromCreated();
  bool advanceToReady();
  bool advanceFromReady();
  bool advanceFromEntered();
  bool advanceFromCompleted();
  bool advanceFromDeparted();
  bool advanceFromArrived();

  void processEntryEvent(const EntryEvent* entryEvent);
  void processExitEvent(const ExitEvent* exitEvent);
  void processChoiceEvent(const ChoiceEvent* choiceEvent);
  void processCompletionEvent(const CompletionEvent* completionEvent);
  void processTriggerEvent(const TriggerEvent* triggerEvent);
  void processMessageDeliveryEvent(const MessageDeliveryEvent* messageDeliveryEvent);

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Token_H

