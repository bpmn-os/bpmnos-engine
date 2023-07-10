#include "JobShop.h"
#include "ResourceActivity.h"

using namespace BPMNOS;

JobShop::JobShop(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::Node(subProcess)
  , BPMN::SubProcess(subProcess,parent)
  , resourceActivity(getResource())
{
}

ResourceActivity* JobShop::getResource() {
  BPMN::SubProcess* ancestor = parent->as<SubProcess>();
  while ( ancestor->parent ) {
    if ( auto resourceActivity = ancestor->represents<ResourceActivity>(); resourceActivity ) {
      return resourceActivity;
    } 
    ancestor = ancestor->parent->represents<SubProcess>();
  }
  throw std::logic_error("JobShop: cannot find resource");
}

