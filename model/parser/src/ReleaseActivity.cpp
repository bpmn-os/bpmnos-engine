#include "ReleaseActivity.h"

using namespace BPMNOS::Model;

ReleaseActivity::ReleaseActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::Node(subProcess)
  , BPMN::FlowNode(subProcess,parent)
  , BPMN::SubProcess(subProcess,parent)
{
}

