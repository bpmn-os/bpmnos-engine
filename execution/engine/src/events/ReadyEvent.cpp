#include "ReadyEvent.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ReadyEvent::ReadyEvent(const Token* token, BPMNOS::Status statusAttributes, BPMNOS::Data dataAttributes)
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
  auto token = this->token.lock();
  if ( !token || expired() ) {
    jsonObject["expired"] = true;
    return jsonObject;
  }
  // the token at a process belongs to the global state machine, which has no process
  jsonObject["processId"] = ( token->owner->root ? token->owner->root->scope : token->node )->id;
  jsonObject["instanceId"] = BPMNOS::to_string(token->getInstanceId(),STRING);
  if ( token->node->represents<BPMN::FlowNode>() ) {
    // the node of the token at a process is the process, which is reported as the process
    jsonObject["nodeId"] = token->node->id;
  }

  return jsonObject;
}
