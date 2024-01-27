#include "ReleaseActivity.h"

using namespace BPMNOS::Model;

ReleaseActivity::ReleaseActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::Node(subProcess)
  , BPMN::FlowNode(subProcess,parent)
  , BPMN::SubProcess(subProcess,parent)
{
  if ( subProcess->extensionElements.has_value() ) {
    releases = extensionElements->get<XML::bpmnos::tAllocations,XML::bpmnos::tRelease>();
  }
}

