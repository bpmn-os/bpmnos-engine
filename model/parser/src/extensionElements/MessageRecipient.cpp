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
    }
  }
}

