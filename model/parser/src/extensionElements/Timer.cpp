#include "Timer.h"
#include "Status.h"
#include "model/utility/src/Keywords.h"
#include "model/parser/src/xml/bpmnos/tTimer.h"
#include "model/parser/src/xml/bpmnos/tParameter.h"

using namespace BPMNOS;

Timer::Timer(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
{
  AttributeMap& attributeMap = parent->extensionElements->as<Status>()->attributeMap;

  if ( element ) {
    for ( XML::bpmnos::tParameter& parameter : get<XML::bpmnos::tTimer,XML::bpmnos::tParameter>() ) {
      if ( parameter.name.value.value == "trigger" ) {
        trigger = std::make_unique<Parameter>(&parameter,attributeMap);
      }
    }
  }
}

number Timer::earliest(const Values& status) const {
    if ( trigger->attribute.has_value() && status[trigger->attribute->get().index].has_value() ) {
      return status[trigger->attribute->get().index].value();
    }
    else if ( trigger->value.has_value() ) {
      return to_number( trigger->value->get().value, ValueType::DECIMAL );
    }
    return status[Status::Index::Timestamp].value();
}  

