#ifndef BPMNOS_Event_H
#define BPMNOS_Event_H

#include "Token.h"

namespace BPMNOS::Execution {

struct Event {
  Event(Token* token);
  virtual ~Event() = 0;
  Token* token;  

  /// Returns a pointer of type T of the Event.
  template<typename T> const T* is() const {
    return dynamic_cast<const T*>(this);
  }

};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Event_H

