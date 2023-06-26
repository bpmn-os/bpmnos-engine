#include "SequenceFlow.h"

using namespace BPMNOS;

SequenceFlow::SequenceFlow(XML::bpmn::tSequenceFlow& sequenceFlow, BPMN::Node* scope)
  : BPMN::SequenceFlow(sequenceFlow,scope)
  , gatekeepers( getExtensionElements<XML::bpmnos::tRestrictions,XML::bpmnos::tRestriction>() )
{
}

