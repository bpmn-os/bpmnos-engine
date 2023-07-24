#include "MessageSender.h"
#include "Parameter.h"
#include "Status.h"

using namespace BPMNOS;

MessageSender::MessageSender(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : Message( baseElement, parent ) 
{
  AttributeMap& attributeMap = parent->extensionElements->as<Status>()->attributeMap;
  for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
    if ( parameter.name.value == "recipient" ) {
      recipient = std::make_unique<Parameter>(&parameter,attributeMap);
    }
  }
}

