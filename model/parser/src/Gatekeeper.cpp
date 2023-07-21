#include "Gatekeeper.h"
#include "Status.h"
#include "xml/bpmnos/tRestrictions.h"
#include "xml/bpmnos/tRestriction.h"

using namespace BPMNOS;

Gatekeeper::Gatekeeper(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , parent(parent)
{
  AttributeMap& attributeMap = parent->extensionElements->as<Status>()->attributeMap;

  for ( XML::bpmnos::tRestriction& restriction : get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() ) {
    try {
      restrictions.push_back( std::make_unique<Restriction>( &restriction,  attributeMap ) );
    }
    catch ( ... ){
      throw std::runtime_error("Gatekeeper: illegal parameters for restriction '" + (std::string)restriction.id + "'");
    }
  }
}

