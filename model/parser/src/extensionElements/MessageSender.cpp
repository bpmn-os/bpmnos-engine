#include "MessageSender.h"
#include "Parameter.h"
#include "Status.h"

using namespace BPMNOS;

MessageSender::MessageSender(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : Message( baseElement, parent ) 
{
  if ( !name.empty() ) {
    AttributeMap& attributeMap = parent->extensionElements->as<Status>()->attributeMap;
    for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
      if ( parameter.name.value.value == "recipient" ) {
        recipient = std::make_unique<Parameter>(&parameter,attributeMap);
      }
    }
  }
}

ValueMap MessageSender::send(const Values& status) const {
  ValueMap message;
  for ( auto& content : contents ) {
    // if the content refers to an attribute that has a value in the status use that value,
    // otherwise use the default value of the message event
    if ( content->attribute.has_value() && status[content->attribute->get().index].has_value() ) {
      message[content->key] = status[content->attribute->get().index].value();
    }
    else if ( content->value.has_value() ) {
      message[content->key] = content->value.value();
    }
  }
  return message;
}

