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
  auto token = this->token.lock();
  if ( !token || expired() ) {
    jsonObject["expired"] = true;
    return jsonObject;
  }
  jsonObject["processId"] = token->owner->root->scope->id;
  jsonObject["instanceId"] = BPMNOS::to_string(token->getInstanceId(),STRING);
  jsonObject["nodeId"] = token->node->id;

  return jsonObject;
}
