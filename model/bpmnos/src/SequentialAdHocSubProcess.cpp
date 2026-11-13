#include "SequentialAdHocSubProcess.h"
#include "Model.h"

using namespace BPMNOS::Model;

SequentialAdHocSubProcess::SequentialAdHocSubProcess(XML::bpmn::tAdHocSubProcess* adHocSubProcess, BPMN::Scope* parent)
  : BPMN::Node(adHocSubProcess)
  , BPMN::FlowNode(adHocSubProcess,parent)
  , BPMN::AdHocSubProcess(adHocSubProcess,parent)
  , performer(nullptr)
{
  if ( !isSequential ) {
    throw std::runtime_error("SequentialAdHocSubProcess: ordering not set to 'Sequential'");
  }

  // determine sequential performer, the nearest node declaring it
  BPMN::Node* node = this;
  while ( true ) {
    if ( auto activity = node->represents<BPMN::Activity>();
      activity && BPMNOS::Model::Model::hasSequentialPerformer( activity->element->resourceRole )
    ) {
      performer = activity;
      break;
    }
    if ( auto process = node->represents<BPMN::Process>() ) {
      if ( BPMNOS::Model::Model::hasSequentialPerformer( process->element->resourceRole ) ) {
        performer = process;
      }
      break;
    }
    node = node->as<BPMN::ChildNode>()->parent;
    if ( node->represents<SequentialAdHocSubProcess>() ) {
      // sequential performer of adhoc subprocess must be found before other adhoc subprocess
      break;
    }
  }

  if ( !performer ) {
    throw std::runtime_error("SequentialAdHocSubProcess: no performer provided for '" + id + "'");
  }
}

