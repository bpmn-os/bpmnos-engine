#include "ResourceActivity.h"

using namespace BPMNOS;

ResourceActivity::ResourceActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::SubProcess(subProcess,parent)
  , BPMN::Node(subProcess)
{
}

