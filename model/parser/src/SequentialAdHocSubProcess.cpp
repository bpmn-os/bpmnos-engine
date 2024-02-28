#include "SequentialAdHocSubProcess.h"
#include "Model.h"

using namespace BPMNOS::Model;

SequentialAdHocSubProcess::SequentialAdHocSubProcess(XML::bpmn::tAdHocSubProcess* adHocSubProcess, BPMN::Scope* parent)
  : BPMN::Node(adHocSubProcess)
  , BPMN::FlowNode(adHocSubProcess,parent)
  , BPMN::AdHocSubProcess(adHocSubProcess,parent)
  , performer(this)
{
  if ( !isSequential ) {
    throw std::runtime_error("SequentialAdHocSubProcess: ordering not set to 'Sequential'");
  }

  // determine sequential performer 
  BPMN::Node* node = this;
  while ( node->represents<BPMN::ChildNode>() ) {
    if ( auto activity = node->represents<BPMN::Activity>() ) {
      if ( BPMNOS::Model::Model::hasSequentialPerformer( activity->element->resourceRole ) ) {
        performer = activity;
        break;
      }
    }
    node = node->as<BPMN::ChildNode>()->parent;
    if ( node->represents<SequentialAdHocSubProcess>() ) {
      // sequential performer of adhoc subprocess must be found before other adhoc subprocess
      break;
    }
  }

  if ( auto process = node->represents<BPMN::Process>() ) {
    if ( BPMNOS::Model::Model::hasSequentialPerformer( process->element->resourceRole ) ) {
      performer = process;
    }
  }
}

