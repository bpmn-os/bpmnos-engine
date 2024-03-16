#include "MessageDeliveryDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryDecision::MessageDeliveryDecision(const Token* token, Message* message, const BPMNOS::Values recipientHeader)
  : Decision(token)
  , message(message)
  , recipientHeader(recipientHeader)
{
}

void MessageDeliveryDecision::processBy(Engine* engine) const {
  engine->process(this);
}
