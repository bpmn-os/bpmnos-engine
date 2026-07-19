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
  std::weak_ptr<const Token> token;  ///< Weak reference to the token; empty for token-less events (clock tick, termination).

  virtual void processBy(Engine* engine) const  = 0;

  /// @brief Returns true if the event has become stale and must be skipped instead of processed.
  /// The default treats a token-bearing event as expired once its token no longer exists; token-less
  /// events and events with additional validity conditions override this.
  virtual bool expired() const;

  /// Returns a pointer of type T of the Event.
  template<typename T> const T* is() const {
    return dynamic_cast<const T*>(this);
  }

  virtual nlohmann::ordered_json jsonify() const = 0;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Event_H

