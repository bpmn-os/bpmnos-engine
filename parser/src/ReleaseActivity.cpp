#include "ReleaseActivity.h"

using namespace BPMNOS;

ReleaseActivity::ReleaseActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::SubProcess(subProcess,parent)
  , BPMN::Node(subProcess)
  , releases( extensionElements->get<XML::bpmnos::tAllocations,XML::bpmnos::tRelease>() )
{
}

