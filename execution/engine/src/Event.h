#ifndef BPMNOS_Execution_Event_H
#define BPMNOS_Execution_Event_H

#include "Token.h"
#include "Observable.h"

namespace BPMNOS::Execution {

class Engine;

//class Event;
//typedef std::vector< std::shared_ptr<Token> > Events;

struct Event : std::enable_shared_from_this<Event>, Observable {
  constexpr Type getObservableType() const override { return Type::Event; };
  Event(const Token* token);
  virtual ~Event() = default;  // Virtual destructor
  const Token* token;  

  virtual bool expired() { return false;}; ///< Utility function allowing specialised events to indicate that they are no longer valid
  virtual void processBy(Engine* engine) const  = 0;

  /// Returns a pointer of type T of the Event.
  template<typename T> const T* is() const {
    return dynamic_cast<const T*>(this);
  }

  virtual nlohmann::ordered_json jsonify() const = 0;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Event_H

