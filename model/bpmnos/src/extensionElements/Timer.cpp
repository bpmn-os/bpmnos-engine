#include "Timer.h"
#include "ExtensionElements.h"
#include "model/utility/src/Keywords.h"
#include "model/bpmnos/src/xml/bpmnos/tTimer.h"
#include "model/bpmnos/src/xml/bpmnos/tParameter.h"

using namespace BPMNOS::Model;

Timer::Timer(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
  , attributeRegistry(parent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry)
{
  if ( element ) {
    for ( XML::bpmnos::tParameter& parameter : get<XML::bpmnos::tTimer,XML::bpmnos::tParameter>() ) {
      if ( parameter.name.value.value == "trigger" ) {
        trigger = std::make_unique<Parameter>(&parameter,attributeRegistry);
      }
    }
  }
}

template <typename DataType>
BPMNOS::number Timer::earliest(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  if ( trigger->expression ) {
    auto value = trigger->expression->execute(status,data,globals);
    if ( value.has_value() ) {
      return value.value();
    }    
  }
  // return current time if no trigger value is available
  return status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value();
}  

template BPMNOS::number Timer::earliest<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template BPMNOS::number Timer::earliest<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;
