#include "Gatekeeper.h"

using namespace BPMNOS;

Gatekeeper::Gatekeeper(XML::bpmn::tBaseElement* baseElement)
  : BPMN::ExtensionElements( baseElement ) 
  , restrictions( get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() )
{
}

