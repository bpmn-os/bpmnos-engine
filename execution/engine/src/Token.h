#ifndef BPMNOS_Execution_Token_H
#define BPMNOS_Execution_Token_H

#include <bpmn++.h>
#include "model/utility/src/Number.h"
#include <nlohmann/json.hpp>

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
private:
  friend class SystemState;
  friend class StateMachine;
  friend class Engine;

public:
  const StateMachine* owner;
  const BPMN::FlowNode* node; 
private:
  enum class State { CREATED, READY, ENTERED, BUSY, COMPLETED, DEPARTED, ARRIVED, DONE, FAILED, TO_BE_MERGED, TO_BE_COPIED };
  static inline std::string stateName[] = { "CREATED", "READY", "ENTERED", "BUSY", "COMPLETED", "DEPARTED", "ARRIVED", "DONE", "FAILED", "TO_BE_MERGED", "TO_BE_COPIED" };
  State state;
public:
  Token(const StateMachine* owner, const BPMN::FlowNode* node, const Values& status);
  Token(const Token* other);
  Values status;

  bool ready() const { return state == State::READY; };
  bool entered() const { return state == State::ENTERED; };
  bool busy() const { return state == State::BUSY; };
  bool completed() const { return state == State::COMPLETED; };
  bool departed() const { return state == State::DEPARTED; };
  bool arrived() const { return state == State::ARRIVED; };
  bool done() const { return state == State::DONE; };
  bool failed() const { return state == State::FAILED; };

  nlohmann::json jsonify() const;
private:

  void run();
  void processEvent(const Event* event);

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

  void update(State newState);

  template <typename F, typename... Args>
    void update(State newState, F&& updateStatus, Args... args) {
    updateStatus(this, std::forward<Args>(args)...);
    update(newState);
  }

  static void appendToStatus(Token* token, const Values& values);
  static void replaceStatus(Token* token, const Values& newStatus);
  static void resizeStatus(Token* token);

  void notify() const;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Token_H

