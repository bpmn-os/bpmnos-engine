#include "CompletionEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

CompletionEvent::CompletionEvent(const Token* token, Values status)
  : Event(token)
  , status(std::move(status))
{
}

void CompletionEvent::processBy(Engine* engine) const {
  engine->process(this);
}

nlohmann::ordered_json CompletionEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "completion";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;

  return jsonObject;
}
