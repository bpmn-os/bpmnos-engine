#include "TimerEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

TimerEvent::TimerEvent(const Token* token)
  : Event(token)
{
}

void TimerEvent::processBy([[maybe_unused]] Engine* engine) const {
  // timer is already processed by engine
}

nlohmann::ordered_json TimerEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "timer";
  jsonObject["processId"] = token->owner->process->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  jsonObject["nodeId"] = token->node->id;

  return jsonObject;
}
