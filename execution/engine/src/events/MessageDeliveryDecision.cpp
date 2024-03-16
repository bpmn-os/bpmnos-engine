#include "MessageDeliveryDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryDecision::MessageDeliveryDecision(const Token* token, Message* message)
  : Decision(token)
  , message(message)
{
}

void MessageDeliveryDecision::processBy(Engine* engine) const {
  engine->process(this);
}
