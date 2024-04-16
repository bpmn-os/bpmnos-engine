#include "MessageDeliveryDecision.h"
#include "execution/engine/src/Engine.h"
#include "execution/controller/src/Evaluator.h"

using namespace BPMNOS::Execution;

MessageDeliveryDecision::MessageDeliveryDecision(const Token* token, const Message* message)
  : Event(token)
  , MessageDeliveryEvent(token,message)
  , Decision()
{
}

std::optional<double> MessageDeliveryDecision::evaluate(Evaluator* evaluator) {
  evaluation = evaluator->evaluate(this);
  return evaluation;
}

