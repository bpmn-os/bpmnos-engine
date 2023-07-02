#include "RequestActivity.h"

using namespace BPMNOS;

RequestActivity::RequestActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::SubProcess(subProcess,parent)
  , BPMN::Node(subProcess)
  , requests( extensionElements->get<XML::bpmnos::tAllocations,XML::bpmnos::tRequest>() )
{
}

