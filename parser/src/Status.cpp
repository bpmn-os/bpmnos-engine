#include "Status.h"

using namespace BPMNOS;

Status::Status(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent)
  : BPMN::ExtensionElements( baseElement ) 
  , status( get<XML::bpmnos::tStatus,XML::bpmnos::tAttribute>() )
  , restrictions( get<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() )
  , operators( get<XML::bpmnos::tOperators,XML::bpmnos::tOperator>() )
{
}
