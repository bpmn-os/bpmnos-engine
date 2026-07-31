#include "MessageDeliveryRequest.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/MessageDefinition.h"
#include <cassert>

using namespace BPMNOS::Execution;

nlohmann::ordered_json MessageDeliveryRequest::jsonify() const {
  auto jsonObject = DecisionRequest::jsonify();

  assert( token->node );
  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  // the definition that built the header, which for a multi-instance activity is the one its status selects
  auto messageDefinition = extensionElements->getMessageDefinition(token->status);
  assert( messageDefinition );

  size_t i = 0;
  for ( auto& [key,type] : messageDefinition->header ) {
    if ( !recipientHeader[i].has_value() ) {
      jsonObject["header"][key] = nullptr;
    }
    else {
      // a value is held only where the parameter states one, and a parameter stating a value states a type
      assert( type.has_value() );
      jsonObject["header"][key] = BPMNOS::to_string(recipientHeader[i].value(),type.value());
    }
    ++i;
  }

  return jsonObject;
}
