#include "Node.h"

using namespace BPMNOS;

Node::Node(XML::bpmn::tProcess& process) 
  : BPMN::Node(process)
  , status( getExtensionElements<XML::bpmnos::tStatus,XML::bpmnos::tAttribute>() )
  , restrictions( getExtensionElements<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() )
  , operators( getExtensionElements<XML::bpmnos::tOperators,XML::bpmnos::tOperator>() )
{
}

Node::Node(XML::bpmn::tFlowNode& flowNode, BPMN::Node* parentNode)
   : BPMN::Node(flowNode,parentNode)
  , status( getExtensionElements<XML::bpmnos::tStatus,XML::bpmnos::tAttribute>() )
  , restrictions( getExtensionElements<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() )
  , operators( getExtensionElements<XML::bpmnos::tOperators,XML::bpmnos::tOperator>() )
{
}

