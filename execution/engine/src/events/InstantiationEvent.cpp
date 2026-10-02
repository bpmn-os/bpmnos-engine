#include "InstantiationEvent.h"
#include "execution/engine/src/Engine.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"

using namespace BPMNOS::Execution;

InstantiationEvent::InstantiationEvent(const BPMN::Process* process, BPMNOS::Values status, BPMNOS::Values data)
  : Event(nullptr)
  , process(process)
  , status(status)
  , data(data)
{
}

void InstantiationEvent::processBy(Engine* engine) const {
  engine->process(this);
}

bool InstantiationEvent::expired() const {
  return false;
}

nlohmann::ordered_json InstantiationEvent::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["event"] = "instantiation";
  jsonObject["processId"] = process->id;
  if ( data.size() > BPMNOS::Model::ExtensionElements::Index::Instance && data[BPMNOS::Model::ExtensionElements::Index::Instance].has_value() ) {
    jsonObject["instanceId"] = BPMNOS::to_string(data[BPMNOS::Model::ExtensionElements::Index::Instance].value(),STRING);
  }

  return jsonObject;
}
