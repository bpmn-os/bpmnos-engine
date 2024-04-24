#include "ExitEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ExitEvent::ExitEvent(const Token* token, std::optional<Values> exitStatus)
  : Event(token)
  , exitStatus(exitStatus)
{
}

void ExitEvent::processBy(Engine* engine) const {
  engine->process(this);
}

nlohmann::ordered_json ExitEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "exit";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;
  jsonObject["state"] = Token::stateName[(int)token->state];

  return jsonObject;
}
