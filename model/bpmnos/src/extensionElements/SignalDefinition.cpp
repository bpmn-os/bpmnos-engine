#include "SignalDefinition.h"
#include "Content.h"
#include "ExtensionElements.h"
#include "model/bpmnos/src/xml/bpmnos/tContent.h"

using namespace BPMNOS::Model;

SignalDefinition::SignalDefinition(XML::bpmnos::tSignal* signal, const AttributeRegistry& attributeRegistry)
  : element(signal)
  , name( BPMNOS::to_number(signal->name.value.value,STRING) )
{
  for ( XML::bpmnos::tContent& content : element->content ) {
    contentMap.emplace(content.key.value.value,std::make_unique<Content>(&content,attributeRegistry));
  }
}
