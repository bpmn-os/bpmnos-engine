#ifndef BPMNOS_Execution_Listener_H
#define BPMNOS_Execution_Listener_H

#include "execution/engine/src/Token.h"

namespace BPMNOS::Execution {

class Listener {
public:
  virtual void update( const Token* token ) = 0;
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Listener_H
