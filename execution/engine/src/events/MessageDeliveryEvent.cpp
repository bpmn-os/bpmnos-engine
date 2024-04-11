#include "MessageDeliveryEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryEvent::MessageDeliveryEvent(const Token* token, const Message* message)
  : Event(token)
  , message(message->weak_from_this())
{
}

bool MessageDeliveryEvent::expired() {
  return message.expired();
}

void MessageDeliveryEvent::processBy(Engine* engine) const {
  engine->process(this);
}
