#include "MessageDeliveryEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryEvent::MessageDeliveryEvent(const Token* token, std::optional<Values> status, std::optional< std::vector< std::unique_ptr<Message> >::const_iterator > message)
  : TriggerEvent(token)
  , status(status)
  , message(message)
{
}

void MessageDeliveryEvent::processBy(Engine* engine) const {
  engine->process(*this);
}
