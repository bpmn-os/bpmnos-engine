#include "Signal.h"

using namespace BPMNOS::Execution;

Signal::Signal(Content content)
  : content(std::move(content))
{
}

nlohmann::ordered_json Signal::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["name"] = BPMNOS::to_string(content.name,BPMNOS::STRING);

  // the content object is rendered as JSON, its fields being named as the type of the signal names them
  jsonObject["content"] = content.object->jsonify();

  return jsonObject;
}
