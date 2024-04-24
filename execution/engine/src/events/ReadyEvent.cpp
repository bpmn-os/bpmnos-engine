#include "ReadyEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ReadyEvent::ReadyEvent(const Token* token, BPMNOS::Values statusAttributes, BPMNOS::Values dataAttributes)
  : Event(token)
  , statusAttributes(statusAttributes)
  , dataAttributes(dataAttributes)
{
}

void ReadyEvent::processBy(Engine* engine) const {
  engine->process(this);
}

nlohmann::ordered_json ReadyEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "ready";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;
  jsonObject["state"] = Token::stateName[(int)token->state];

  return jsonObject;
}
