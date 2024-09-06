#include "Gatekeeper.h"
#include "ExtensionElements.h"
#include "model/bpmnos/src/xml/bpmnos/tRestrictions.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"

using namespace BPMNOS::Model;

Gatekeeper::Gatekeeper(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
{
  AttributeRegistry& attributeRegistry = parent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry;
  for ( XML::bpmnos::tRestriction& condition : get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() ) {
    conditions.push_back( std::make_unique<Restriction>( &condition,  attributeRegistry ) );
  }
}

template <typename DataType>
bool Gatekeeper::conditionsSatisfied(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  for ( auto& condition : conditions ) {
    if ( !condition->isSatisfied(status,data,globals) ) {
      return false; 
    }
  }
  return true; 
}

template bool Gatekeeper::conditionsSatisfied<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template bool Gatekeeper::conditionsSatisfied<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;
