#ifndef BPMNOS_Execution_SequentialPerformerUpdate_H
#define BPMNOS_Execution_SequentialPerformerUpdate_H

#include "Token.h"
#include "Observable.h"

namespace BPMNOS::Execution {


struct SequentialPerformerUpdate : Observable {
  constexpr Type getObservableType() const override { return Type::SequentialPerformerUpdate; };
  SequentialPerformerUpdate(const Token* t) : token(t) {}
  const Token* token;  
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_SequentialPerformerUpdate_H

