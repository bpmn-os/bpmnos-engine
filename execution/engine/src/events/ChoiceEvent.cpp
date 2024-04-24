#include "ChoiceEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ChoiceEvent::ChoiceEvent(const Token* token, Values updatedStatus)
  : Event(token)
  , updatedStatus(updatedStatus)
{
}

void ChoiceEvent::processBy(Engine* engine) const {
  engine->process(this);
}

nlohmann::ordered_json ChoiceEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "choice";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;

  return jsonObject;
}
