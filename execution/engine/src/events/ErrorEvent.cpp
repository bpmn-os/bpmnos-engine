#include "ErrorEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ErrorEvent::ErrorEvent(const Token* token)
  : Event(token)
{
}

void ErrorEvent::processBy(Engine* engine) const {
  engine->process(this);
}

nlohmann::ordered_json ErrorEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "error";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;
  jsonObject["state"] = Token::stateName[(int)token->state];

  return jsonObject;
}
