#include "Signal.h"

using namespace BPMNOS::Execution;

Signal::Signal(BPMNOS::number name, BPMNOS::VariedValueMap content)
  : name(name)
  , content(std::move(content))
{
}

nlohmann::ordered_json Signal::jsonify() const {
  nlohmann::ordered_json jsonObject;

  jsonObject["name"] = BPMNOS::to_string(name,BPMNOS::STRING);

  // the content is rendered as it is held: a signal states no types, the keys being mapped to attributes of
  // the recipient rather than of the signal, and every recipient may map the same key to a type of its own
  for ( auto& [key,contentValue] : content ) {
    if ( std::holds_alternative< std::string >(contentValue) ) {
      jsonObject["content"][key] = std::get< std::string >(contentValue);
    }
    else if ( auto& value = std::get< BPMNOS::Value >(contentValue); value.has_value() ) {
      jsonObject["content"][key] = (double)value.value();
    }
    else {
      jsonObject["content"][key] = nullptr;
    }
  }

  return jsonObject;
}
