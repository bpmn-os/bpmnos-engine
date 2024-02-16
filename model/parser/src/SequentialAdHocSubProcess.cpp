#include "SequentialAdHocSubProcess.h"
#include <iostream>

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

  auto hasSequentialPerformer = [](const std::vector< std::reference_wrapper<XML::bpmn::tResourceRole> >& resources) {
    for ( auto& resource : resources ) {
      if ( auto performer = resource.get().get<XML::bpmn::tPerformer>();
        performer && performer->name.has_value() && performer->name.value().get().value.value == "Sequential"
      ) {
        return true;
      } 
    }
    return false;
  };

  // determine sequential performer 
  BPMN::Node* node = this;
  while ( node->represents<BPMN::ChildNode>() ) {
    if ( auto activity = node->represents<BPMN::Activity>() ) {
      if ( hasSequentialPerformer( activity->element->resourceRole ) ) {
        performer = activity;
        break;
      }
    }
    node = node->as<BPMN::ChildNode>()->parent;
  }

  if ( auto process = node->represents<BPMN::Process>() ) {
    if ( hasSequentialPerformer( process->element->resourceRole ) ) {
      performer = process;
    }
  }
}

