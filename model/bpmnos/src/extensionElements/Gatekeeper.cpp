#include "Gatekeeper.h"
#include "ExtensionElements.h"
#include "model/bpmnos/src/xml/bpmnos/tRestrictions.h"
#include "model/bpmnos/src/xml/bpmnos/tRestriction.h"

using namespace BPMNOS::Model;

Gatekeeper::Gatekeeper(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
{
  AttributeMap& statusAttributes = parent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->statusAttributes;
  for ( XML::bpmnos::tRestriction& restriction : get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() ) {
    restrictions.push_back( std::make_unique<Restriction>( &restriction,  statusAttributes ) );
  }
}

bool Gatekeeper::restrictionsSatisfied(const Values& values) const {
  for ( auto& restriction : restrictions ) {
    if ( !restriction->isSatisfied(values) ) {
      return false; 
    }
  }
  return true; 
}

