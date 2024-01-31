#include "Sequencer.h"
#include "ResourceActivity.h"

using namespace BPMNOS::Model;

Sequencer::Sequencer(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent)
  : BPMN::Node(subProcess)
  , BPMN::FlowNode(subProcess,parent)
  , BPMN::SubProcess(subProcess,parent)
  , reference(this)
{
  // determine whether sequencer reference should be set to resource activity 
  BPMN::ChildNode* ancestor = parent->represents<BPMN::ChildNode>();
  while ( ancestor ) {
    if ( ancestor->represents<Sequencer>() ) {
      if ( auto resourceActivity = ancestor->represents<ResourceActivity>(); resourceActivity ) {
        reference = resourceActivity;
      }
      break;
    }
    ancestor = ancestor->parent->represents<ChildNode>();
  }
}

const std::vector<BPMN::Activity*>& Sequencer::getJobs() {
  return reference->jobs;
}

void Sequencer::addJob(BPMN::Activity* job) {
  reference->jobs.push_back(job);
}

