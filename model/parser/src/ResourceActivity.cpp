#include "ResourceActivity.h"

using namespace BPMNOS;

ResourceActivity::ResourceActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::Node(subProcess)
  , BPMN::SubProcess(subProcess,parent)
{
}

