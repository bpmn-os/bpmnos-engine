#include "ExitEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ExitEvent::ExitEvent(const Token* token, std::optional<Status> exitStatus)
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
