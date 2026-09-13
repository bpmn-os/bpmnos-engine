#include "SignalDefinition.h"
#include "ExtensionElements.h"
#include "model/utility/src/Keywords.h"
#include "model/bpmnos/src/xml/bpmnos/tSignal.h"
#include "model/bpmnos/src/xml/bpmnos/tParameter.h"
#include "model/bpmnos/src/xml/bpmnos/tSignal.h"

using namespace BPMNOS::Model;

SignalDefinition::SignalDefinition(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement )
  , parent(parent)
  , attributeRegistry(parent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry)
  , signal(nullptr)
{
  if ( !element ) return;

  // get signal definition
  if ( auto definition = element->getOptionalChild<XML::bpmnos::tSignal>(); definition.has_value() ) {
    signal = &definition.value().get();
    name = BPMNOS::to_number(signal->name.value.value,STRING);

    for ( XML::bpmnos::tContent& content : signal->content ) {
      contentMap.emplace(content.key.value.value,std::make_unique<Content>(&content,attributeRegistry));
    }

    if ( baseElement->is<XML::bpmn::tCatchEvent>() ) {
      // add data attributes modified by signal content to dataUpdate
      for ( auto& [key,content] : contentMap ) {
        Attribute* attribute = content->attribute;
        if ( attribute->category != Attribute::Category::STATUS ) {
          dataUpdate.attributes.push_back(attribute);
        }
        if ( attribute->category == Attribute::Category::GLOBAL ) {
          dataUpdate.global = true;
        }
      }
    }
  }
}

