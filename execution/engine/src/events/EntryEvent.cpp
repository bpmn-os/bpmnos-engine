#include "EntryEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

EntryEvent::EntryEvent(const Token* token, std::optional<Values> entryStatus)
  : Event(token)
  , entryStatus(entryStatus)
{
}

void EntryEvent::processBy(Engine* engine) const {
  engine->process(this);
}

nlohmann::ordered_json EntryEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "entry";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;

  return jsonObject;
}
