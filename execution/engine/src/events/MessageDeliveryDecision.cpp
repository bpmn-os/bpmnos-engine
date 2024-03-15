#include "MessageDeliveryDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryDecision::MessageDeliveryDecision(const Token* token, const BPMNOS::Values& recipientHeader, Message* message)
  : Decision(token)
  , recipientHeader(recipientHeader)
  , message(message)
{
}

void MessageDeliveryDecision::processBy(Engine* engine) const {
  engine->process(this);
}
