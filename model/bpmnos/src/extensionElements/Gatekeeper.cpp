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
  for ( XML::bpmnos::tRestriction& restriction : get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() ) {
    restrictions.push_back( std::make_unique<Restriction>( &restriction,  attributeRegistry ) );
  }
}

template <typename DataType>
bool Gatekeeper::restrictionsSatisfied(const BPMNOS::Values& status, const DataType& data) const {
  for ( auto& restriction : restrictions ) {
    if ( !restriction->isSatisfied(status,data) ) {
      return false; 
    }
  }
  return true; 
}

template bool Gatekeeper::restrictionsSatisfied<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data) const;
template bool Gatekeeper::restrictionsSatisfied<BPMNOS::Globals>(const BPMNOS::Values& status, const BPMNOS::Globals& data) const;
