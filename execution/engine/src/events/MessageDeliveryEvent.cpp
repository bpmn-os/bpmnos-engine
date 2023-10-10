#include "MessageDeliveryEvent.h"

using namespace BPMNOS::Execution;

MessageDeliveryEvent::MessageDeliveryEvent(Token* token, std::optional<Values> status, std::optional< std::vector< std::unique_ptr<Message> >::const_iterator > message)
  : TriggerEvent(token)
  , status(status)
  , message(message)
{
}

