#include "MessageDeliveryEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryEvent::MessageDeliveryEvent(const Token* token, const BPMNOS::Values& recipientHeader, Message* message)
  : Event(token)
  , recipientHeader(recipientHeader)
  , message(message)
{
}

void MessageDeliveryEvent::processBy(Engine* engine) const {
  engine->process(this);
}
