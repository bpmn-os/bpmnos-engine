#include "MessageDeliveryEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

MessageDeliveryEvent::MessageDeliveryEvent(const Token* token, const Message* message)
  : Event(token)
  , message(message->weak_from_this())
{
}

void MessageDeliveryEvent::processBy(Engine* engine) const {
  engine->process(this);
}

bool MessageDeliveryEvent::expired() const {
  // Stale if the recipient token no longer exists or the message was withdrawn (erased from the pool).
  return token.expired() || message.expired();
}

nlohmann::ordered_json MessageDeliveryEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "messagedelivery";
  auto token = this->token.lock();
  auto message = this->message.lock();
  if ( !token || !message || expired() ) {
    jsonObject["expired"] = true;
    return jsonObject;
  }
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;
  jsonObject["message"] = message->jsonify();
  
  return jsonObject;
}
