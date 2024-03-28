#include "Data.h"
#include "ExtensionElements.h"
#include "model/bpmnos/src/xml/bpmnos/tAttributes.h"
#include "model/bpmnos/src/xml/bpmnos/tAttribute.h"

using namespace BPMNOS::Model;

Data::Data(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* scope)
  : BPMN::ExtensionElements( baseElement ) 
  , scope(scope)
{
  if ( !element ) return; 
  auto extensionElements = scope->extensionElements->as<BPMNOS::Model::ExtensionElements>();

  if ( auto elements = element->getOptionalChild<XML::bpmnos::tAttributes>();
    elements &&
    elements.has_value()
  ) {
    for ( XML::bpmnos::tAttribute& attributeElement : elements.value().get().attribute ) {
      attributes.emplace_back( std::make_unique<Attribute>(&attributeElement, Attribute::Category::DATA, extensionElements->attributeRegistry) );
      extensionElements->data.push_back(attributes.back().get());
    }
  }
}

