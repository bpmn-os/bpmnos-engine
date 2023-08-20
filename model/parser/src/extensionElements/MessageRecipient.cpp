#include "MessageRecipient.h"
#include "Parameter.h"
#include "Status.h"

using namespace BPMNOS;

MessageRecipient::MessageRecipient(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : Message( baseElement, parent ) 
{
  if ( !name.empty() ) {
    AttributeMap& attributeMap = parent->extensionElements->as<Status>()->attributeMap;
    for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
      if ( parameter.name.value.value == "sender" ) {
        sender = std::make_unique<Parameter>(&parameter,attributeMap);
      }
    }
    for ( auto& content : contents ) {
      if ( !content->attribute.has_value() ) {
        throw std::runtime_error("MessageRecipient: missing attribute for content '" + content->id + "'");
      }
      content->attribute->get().isImmutable = false;
    }
  }
}

void MessageRecipient::receive(Values& status, const ValueMap& message) const {
  for ( auto& content : contents ) {
    // use the value of the message if available, 
    // otherwise use the default value of the message event if available, 
    // otherwise set status value to undefined
    if ( message.contains(content->key) && message.at(content->key).has_value() ) {
      status[content->attribute->get().index] = message.at(content->key).value();
    }
    else if ( content->value.has_value() ) {
      status[content->attribute->get().index] = content->value.value();      
    }
    else {
      status[content->attribute->get().index] = std::nullopt;
    }
  }
}
