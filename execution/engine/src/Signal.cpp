#include "Signal.h"

using namespace BPMNOS::Execution;

Signal::Signal(BPMNOS::number name, std::shared_ptr<const BPMNOS::Object> content)
  : name(name)
  , content(std::move(content))
{
}

nlohmann::ordered_json Signal::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["name"] = BPMNOS::to_string(name,BPMNOS::STRING);

  // the content object is rendered as JSON, its fields being named as the type of the signal names them
  jsonObject["content"] = BPMNOS::to_json(*content);

  return jsonObject;
}
