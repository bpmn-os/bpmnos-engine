#ifndef BPMNOS_Execution_Event_H
#define BPMNOS_Execution_Event_H

#include "Token.h"

namespace BPMNOS::Execution {

class Engine;

struct Event {
  Event(const Token* token);
  virtual ~Event() = default;  // Virtual destructor
  const Token* token;  

  virtual void processBy(Engine* engine) const  = 0;

  /// Returns a pointer of type T of the Event.
  template<typename T> const T* is() const {
    return dynamic_cast<const T*>(this);
  }

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Event_H

