#include "MessageDeliveryEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryEvent::MessageDeliveryEvent(const Token* token, Messages::const_iterator message_it)
  : Event(token)
  , message_it(message_it)
{
}

void MessageDeliveryEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
