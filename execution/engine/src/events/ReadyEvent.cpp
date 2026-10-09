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
  auto token = this->token.lock();
  if ( !token || expired() ) {
    jsonObject["expired"] = true;
    return jsonObject;
  }
  jsonObject["processId"] = token->owner->root->scope->id;
  jsonObject["instanceId"] = BPMNOS::to_string((*token->data)[BPMNOS::Model::ExtensionElements::Index::Instance].get().value(),STRING);
  if ( token->node->represents<BPMN::FlowNode>() ) {
    // the node of the token at a process is the process, which is reported as the process
    jsonObject["nodeId"] = token->node->id;
  }

  return jsonObject;
}
