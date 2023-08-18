#include "RequestActivity.h"

using namespace BPMNOS;

RequestActivity::RequestActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::Node(subProcess)
  , BPMN::SubProcess(subProcess,parent)
{
  if ( subProcess->extensionElements.has_value() ) {
    requests = extensionElements->get<XML::bpmnos::tAllocations,XML::bpmnos::tRequest>();
  }
}

