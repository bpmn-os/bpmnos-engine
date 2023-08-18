#include "Timer.h"
#include "Status.h"
#include "model/parser/src/xml/bpmnos/tRestrictions.h"
#include "model/parser/src/xml/bpmnos/tRestriction.h"

using namespace BPMNOS;

Timer::Timer(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
{
  AttributeMap& attributeMap = parent->extensionElements->as<Status>()->attributeMap;
  if ( element ) {
    for ( XML::bpmnos::tParameter& parameter : element->getChildren<XML::bpmnos::tParameter>() ) {
      if ( parameter.name.value == "trigger" ) {
        trigger = std::make_unique<Parameter>(&parameter,attributeMap);
      }
    }
  }
}

