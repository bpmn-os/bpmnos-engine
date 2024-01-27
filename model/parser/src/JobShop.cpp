#include "JobShop.h"
#include "ResourceActivity.h"

using namespace BPMNOS::Model;

JobShop::JobShop(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::Node(subProcess)
  , BPMN::FlowNode(subProcess,parent)
  , BPMN::SubProcess(subProcess,parent)
  , resourceActivity(initializeResource())
{
}

ResourceActivity* JobShop::initializeResource() {
  BPMN::ChildNode* ancestor = parent->as<BPMN::ChildNode>();
  while ( ancestor ) {
    if ( auto resourceActivity = ancestor->represents<ResourceActivity>(); resourceActivity ) {
      return resourceActivity;
    } 
    ancestor = ancestor->parent->represents<ChildNode>();
  }
  throw std::logic_error("JobShop: cannot find resource");
}
