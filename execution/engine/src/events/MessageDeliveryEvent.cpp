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

nlohmann::ordered_json MessageDeliveryEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "messagedelivery";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;

  if ( auto object = message.lock() ) {
    jsonObject["message"] = object->jsonify();
  }
  
  return jsonObject;
}
