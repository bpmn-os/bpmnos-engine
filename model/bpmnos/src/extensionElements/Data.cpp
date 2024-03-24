#include "Data.h"
#include "ExtensionElements.h"
#include "model/bpmnos/src/xml/bpmnos/tStatus.h"
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"

using namespace BPMNOS::Model;

Data::Data(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* scope)
  : BPMN::ExtensionElements( baseElement ) 
  , scope(scope)
{
  if ( !element ) return; 

  if ( auto status = element->getOptionalChild<XML::bpmnos::tStatus>(); status.has_value() ) {
    // add all attributes
    if ( status->get().attributes.has_value() ) {
      for ( XML::bpmnos::tAttribute& attributeElement : status->get().attributes.value().get().attribute ) {
        auto attribute = std::make_unique<Attribute>(&attributeElement,attributeMap);
        attributeMap[attribute->name] = attribute.get();
        attributes.push_back(std::move(attribute));
      }
    }
  }
}

