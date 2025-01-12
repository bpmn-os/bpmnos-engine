#include "Signal.h"
#include "ExtensionElements.h"
#include "model/utility/src/Keywords.h"
#include "model/bpmnos/src/xml/bpmnos/tSignal.h"
#include "model/bpmnos/src/xml/bpmnos/tParameter.h"
#include "model/bpmnos/src/xml/bpmnos/tSignal.h"

using namespace BPMNOS::Model;

Signal::Signal(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
  , attributeRegistry(parent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry)
{
  if ( !element ) return;
  
  // get signal definition
  if ( auto signal = element->getOptionalChild<XML::bpmnos::tSignal>(); signal.has_value() ) {
    name = BPMNOS::to_number(signal.value().get().name.value.value,STRING);
  
    for ( XML::bpmnos::tContent& content : signal.value().get().content ) {
      contentMap.emplace(content.key.value.value,std::make_unique<Content>(&content,attributeRegistry));
    }

    if ( baseElement->is<XML::bpmn::tCatchEvent>() ) {
      // add data attributes modified by signal to dataUpdateOnCompletion (global values must not be updated by signals)
      for ( auto& [key,content] : contentMap ) {
        Attribute* attribute = content->attribute;
        if ( attribute->category == Attribute::Category::DATA ) {
          updatedData.push_back(attribute);
        }
        else if ( attribute->category == Attribute::Category::GLOBAL ) {
          throw std::runtime_error("Signal: illegal update of global attribute'" + (std::string)attribute->id + "' for content '" + (std::string)content->id + "'.");
        }
      }
    }
  }
}

