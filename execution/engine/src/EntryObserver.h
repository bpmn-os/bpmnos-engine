#ifndef BPMNOS_Execution_EntryObserver_H
#define BPMNOS_Execution_EntryObserver_H

#include "Observable.h"
#include "Observer.h"
#include <memory>
#include <vector>
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "execution/utility/src/auto_list.h"

namespace BPMNOS::Execution {

class SystemState;
class Token;
struct Event;

/**
 * @brief Enters the tokens at activities whose entry is not decided as soon as their entry restrictions hold.
 *
 * Only the entry of an activity within a sequential ad-hoc subprocess is decided, by the performer of the
 * subprocess. Any other activity is entered as soon as the entry restrictions, those of scope `entry` or `full`
 * and those inherited from the enclosing scopes, hold on the status the token has with the current time as its
 * timestamp, as a conditional event is triggered as soon as its condition holds. A token whose restrictions do not
 * hold waits among the tokens awaiting entry of the system state. It is checked again whenever a data update
 * concerns an attribute its restrictions depend on and, if they depend on the timestamp, whenever time advances.
 * A token whose restrictions never hold waits until the run ends.
 *
 * The observer supplies one entry event at a time, which the engine dispatches before the events of the
 * controller, so that the entry is notified and processed like any other event.
 */
class EntryObserver : public Observer {
public:
  EntryObserver();
  /**
   * @brief Binds the observer to a system state, every token awaiting entry in it being checked again.
   */
  void connect(SystemState* systemState);
  /**
   * @brief Registers a token that has become ready at an activity whose entry is not decided.
   */
  void await(Token* token);
  void notice(const Observable* observable) override;
  /**
   * @brief Returns the entry event of a token whose entry restrictions hold, or a null pointer if there is none.
   */
  std::shared_ptr<Event> dispatchEvent();
protected:
  SystemState* systemState;
  BPMNOS::number time; ///< The time at which the tokens whose restrictions depend on the timestamp were last checked
  auto_list< std::weak_ptr<Token> > candidates; ///< Tokens awaiting entry to be checked at the next dispatch
  void schedule(const std::vector<const BPMNOS::Model::Attribute*>& attributes, auto_list< std::weak_ptr<Token> >& waitingTokens);
  static bool isTimeDependent(const Token* token); ///< Returns true if the entry restrictions of the token depend on the timestamp
};

} // namespace BPMNOS::Execution
#endif // BPMNOS_Execution_EntryObserver_H
