#include "SequentialAdHocSubProcess.h"
#include "ResourceActivity.h"

using namespace BPMNOS::Model;

SequentialAdHocSubProcess::SequentialAdHocSubProcess(XML::bpmn::tAdHocSubProcess* adHocSubProcess, BPMN::Scope* parent)
  : BPMN::Node(adHocSubProcess)
  , BPMN::FlowNode(adHocSubProcess,parent)
  , BPMN::AdHocSubProcess(adHocSubProcess,parent)
  , sequencer(this)
{
  if ( !isSequential ) {
    throw std::runtime_error("SequentialAdHocSubProcess: ordering not set to 'Sequential'");
  }

  // determine whether sequencer is resource activity 
  BPMN::ChildNode* ancestor = parent->represents<BPMN::ChildNode>();
  while ( ancestor ) {
    if ( ancestor->represents<SequentialAdHocSubProcess>() ) {
      break;
    }
    else if ( auto resourceActivity = ancestor->represents<ResourceActivity>() ) {
      sequencer = resourceActivity;
      break;
    }
    ancestor = ancestor->parent->represents<ChildNode>();
  }
}

/*
const std::vector<BPMN::Activity*>& SequentialAdHocSubProcess::getJobs() {
  return sequencer->jobs;
}

void SequentialAdHocSubProcess::addJob(BPMN::Activity* job) {
  sequencer->jobs.push_back(job);
}
*/

