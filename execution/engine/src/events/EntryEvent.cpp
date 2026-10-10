#include "EntryEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

EntryEvent::EntryEvent(const Token* token, std::optional<Status> entryStatus)
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
