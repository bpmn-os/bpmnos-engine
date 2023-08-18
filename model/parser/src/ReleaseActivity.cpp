#include "ReleaseActivity.h"

using namespace BPMNOS;

ReleaseActivity::ReleaseActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::Node(subProcess)
  , BPMN::SubProcess(subProcess,parent)
{
  if ( subProcess->extensionElements.has_value() ) {
    releases = extensionElements->get<XML::bpmnos::tAllocations,XML::bpmnos::tRelease>();
  }
}

