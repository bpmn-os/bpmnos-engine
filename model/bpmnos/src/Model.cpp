#include <unordered_set>
#include <cassert>
#include <iostream>

#include "Model.h"
#include "extensionElements/ExtensionElements.h"
#include "extensionElements/Gatekeeper.h"
#include "extensionElements/Timer.h"
#include "extensionElements/MessageDefinition.h"
#include "DecisionTask.h"
#include "SequentialAdHocSubProcess.h"

using namespace BPMNOS::Model;

Model::Model(const std::string& filename)
{
  readBPMNFile(filename);
}

std::unique_ptr<BPMN::Process> Model::createProcess(XML::bpmn::tProcess* process) {
  // bind attributes, restrictions, and operators to all processes
  return bind<BPMN::Process>(
    BPMN::Model::createProcess(process),
    std::make_unique<BPMNOS::Model::ExtensionElements>(process)
  );
}

std::unique_ptr<BPMN::EventSubProcess> Model::createEventSubProcess(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent) {
  // bind attributes, restrictions, and operators to all processes
  return bind<BPMN::EventSubProcess>(
    BPMN::Model::createEventSubProcess(subProcess,parent),
    std::make_unique<BPMNOS::Model::ExtensionElements>(subProcess,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createActivity(XML::bpmn::tActivity* activity, BPMN::Scope* parent) {
  auto node =  bind<BPMN::FlowNode>(
    BPMN::Model::createActivity(activity,parent),
    std::make_unique<BPMNOS::Model::ExtensionElements>(activity,parent)
  );
  if ( auto adHocSubProcess = node->represents<SequentialAdHocSubProcess>();
    adHocSubProcess && adHocSubProcess->performer == adHocSubProcess
  ) {
    // set flag in case performer is not explicitly provided
    node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->hasSequentialPerformer = true;
  }
  return node;
}

std::unique_ptr<BPMN::SequenceFlow> Model::createSequenceFlow(XML::bpmn::tSequenceFlow* sequenceFlow, BPMN::Scope* scope) {
  // bind gatekeeper restrictions to all sequence flows
  return bind<BPMN::SequenceFlow>(
    BPMN::Model::createSequenceFlow(sequenceFlow,scope),
    std::make_unique<Gatekeeper>(sequenceFlow,scope)
  );
}

std::unique_ptr<BPMN::DataObject> Model::createDataObject(XML::bpmn::tDataObject* dataObject, BPMN::Scope* scope) {
  // bind attributes to all data objects
  return bind<BPMN::DataObject>(
    BPMN::Model::createDataObject(dataObject,scope),
    std::make_unique<BPMNOS::Model::ExtensionElements>(dataObject,scope)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createAdHocSubProcess(XML::bpmn::tAdHocSubProcess* adHocSubProcess, BPMN::Scope* parent) {
  return std::make_unique<SequentialAdHocSubProcess>(adHocSubProcess,parent);
}

std::unique_ptr<BPMN::FlowNode> Model::createTask(XML::bpmn::tTask* task, BPMN::Scope* parent) {
  if ( const auto& type = task->getOptionalAttributeByName("type"); 
       type.has_value() && type->get().xmlns == "https://bpmnos.telematique.eu" 
  ) {
    if ( type->get().value.value == "Decision" ) {
      // decisions are added with status
      return std::make_unique<DecisionTask>(task,parent);
    }
    else {
      throw std::runtime_error("Model: Illegal type '" + (std::string)type->get().value + "'");
    }   
  }
  return BPMN::Model::createTask(task, parent);
}


std::unique_ptr<BPMN::FlowNode> Model::createTimerStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) {
  // bind timer
  return bind<BPMN::FlowNode>(
    BPMN::Model::createTimerStartEvent(startEvent,parent),
    std::make_unique<Timer>(startEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createTimerBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  // bind timer
  return bind<BPMN::FlowNode>(
    BPMN::Model::createTimerBoundaryEvent(boundaryEvent,parent),
    std::make_unique<Timer>(boundaryEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createTimerCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  // bind timer
  return bind<BPMN::FlowNode>(
    BPMN::Model::createTimerCatchEvent(catchEvent,parent),
    std::make_unique<Timer>(catchEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageStartEvent(XML::bpmn::tStartEvent* startEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageStartEvent(startEvent,parent),
    std::make_unique<BPMNOS::Model::ExtensionElements>(startEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageBoundaryEvent(boundaryEvent,parent),
    std::make_unique<BPMNOS::Model::ExtensionElements>(boundaryEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageCatchEvent(catchEvent,parent),
    std::make_unique<BPMNOS::Model::ExtensionElements>(catchEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageThrowEvent(throwEvent,parent),
    std::make_unique<BPMNOS::Model::ExtensionElements>(throwEvent,parent)
  );
}

void Model::createMessageFlows() {
  BPMN::Model::createMessageFlows();

  // Messages can only flow between message extensions with the same name and header.
  // If message flows are given in the model, the message flow closest to the meesage
  // event restricts the candidate catching or throwing message events.
  for ( auto& sendingProcess : processes ) {
    // find all throwing message events of the sending process
    auto throwingMessageEvents = sendingProcess->find_all(
      [](const BPMN::Node* node) { return node->represents<BPMN::MessageThrowEvent>();}
    );

    for ( auto& receivingProcess : processes ) {
      // only consider node pairs belonging to different processes
      if ( sendingProcess.get() != receivingProcess.get() ) {
        // find all catching message events of receiving process
        auto catchingMessageEvents = receivingProcess->find_all(
         [](const BPMN::Node* node) { return node->represents<BPMN::MessageCatchEvent>();}
        );

        for ( auto throwingMessageEvent : throwingMessageEvents ) {
          for ( auto catchingMessageEvent : catchingMessageEvents ) {
            createMessageCandidates(sendingProcess.get(), throwingMessageEvent->as<BPMN::FlowNode>(), receivingProcess.get(), catchingMessageEvent->as<BPMN::FlowNode>());
          }
        }
      }

    }
  }

}

std::vector<BPMN::MessageFlow*>& Model::determineMessageFlows(BPMN::FlowNode* messageEvent, auto getMessageFlows) {
  auto& relevantFlows = getMessageFlows(messageEvent);
  if ( relevantFlows.empty() ) {
    BPMN::ChildNode* node = messageEvent;
    BPMN::Scope* scope = nullptr;
    do {
      // get next scope that may have message flows
      scope = node->parent;
      while ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>() ) {
        // skip event-subprocesses
        node = eventSubProcess;
        scope = eventSubProcess->parent;
      }

      relevantFlows = getMessageFlows(scope);
      node = scope->represents<BPMN::SubProcess>();

    } while ( relevantFlows.empty() && node);
  }
  return relevantFlows;
}
bool Model::messageMayBeCaught( [[maybe_unused]] BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent ) {
  // determine relevant message flows for throwing events
  auto& outgoingMessageFlows = determineMessageFlows(
    throwingMessageEvent, 
    [](BPMN::Node* node) -> std::vector<BPMN::MessageFlow*>& {
      return node->sending;
    }
  );

  if ( outgoingMessageFlows.empty() ) {
    // no message flow is provided that imposes a restriction
    return true;
  }

  // determine whether catching message event is in message flow target
  bool found = false;
  for ( auto messageFlow : outgoingMessageFlows ) {
    auto& [process,flowNode] = messageFlow->target;
    if ( process == receivingProcess ) {
      if ( flowNode == catchingMessageEvent ) {
        return true;
      }
      else {
        if ( flowNode ) {
          found = flowNode->find_all(
            [catchingMessageEvent](const BPMN::Node* node) { return node == catchingMessageEvent;}
          ).size();
        }
        else {
          found = process->find_all(
            [catchingMessageEvent](const BPMN::Node* node) { return node == catchingMessageEvent;}
          ).size();
        }
      }
      if ( found ) {
        return true;
      }
    }
  }
  return false;
}

bool Model::messageMayBeThrown( BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, [[maybe_unused]] BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent ) {
  // determine relevant message flows for catching event
  auto incomingMessageFlows = determineMessageFlows(
    catchingMessageEvent, 
    [](BPMN::Node* node) -> std::vector<BPMN::MessageFlow*>& {
      return node->receiving;
    }
  );

  if ( incomingMessageFlows.empty() ) {
    // no message flow is provided that imposes a restriction
    return true;
  }

  // determine whether throwing message event is in message flow source
  bool found = false;
  for ( auto messageFlow : incomingMessageFlows ) {
    auto& [process,flowNode] = messageFlow->source;
    if ( process == sendingProcess ) {
      if ( flowNode == throwingMessageEvent ) {
        return true;
      }
      else {
        if ( flowNode ) {
          found = flowNode->find_all(
            [throwingMessageEvent](const BPMN::Node* node) { return node == throwingMessageEvent;}
          ).size();
        }
        else {
          found = process->find_all(
            [throwingMessageEvent](const BPMN::Node* node) { return node == throwingMessageEvent;}
          ).size();
        }
        if ( found ) {
          return true;
        }
      }
    }
  }
  return false;
}

void Model::createMessageCandidates( BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent ) {
  auto senderExtension = throwingMessageEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  for ( auto& outgoingMessageDefinition : senderExtension->messageDefinitions ) {
    auto recipientExtension = catchingMessageEvent->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
    for ( auto& incomingMessageDefinition : recipientExtension->messageDefinitions) {

      assert( outgoingMessageDefinition.get() );
      assert( incomingMessageDefinition.get() );

      if ( outgoingMessageDefinition->name != incomingMessageDefinition->name ) {
        continue;
      }
      if ( outgoingMessageDefinition->header != incomingMessageDefinition->header ) {
        continue;
      }

      if ( messageMayBeCaught(sendingProcess, throwingMessageEvent, receivingProcess, catchingMessageEvent) &&
        messageMayBeThrown(sendingProcess, throwingMessageEvent, receivingProcess, catchingMessageEvent)
      ) {
        // add message events to collection of candidates of each other
        if( find(
            senderExtension->messageCandidates.begin(),
            senderExtension->messageCandidates.end(),
            catchingMessageEvent->as<BPMN::FlowNode>()
          ) == senderExtension->messageCandidates.end()
        ) {
//std::cerr << throwingMessageEvent->id << " -> " << catchingMessageEvent->id << std::endl;
          senderExtension->messageCandidates.push_back(catchingMessageEvent->as<BPMN::FlowNode>());
        }

        if( find(
            recipientExtension->messageCandidates.begin(),
            recipientExtension->messageCandidates.end(),
            throwingMessageEvent->as<BPMN::FlowNode>()
          ) == recipientExtension->messageCandidates.end()
        ) {
//std::cerr << throwingMessageEvent->id << " -> " << catchingMessageEvent->id << std::endl;
          recipientExtension->messageCandidates.push_back(throwingMessageEvent->as<BPMN::FlowNode>());
        }
      }
    }
  }
}

bool Model::hasSequentialPerformer(const std::vector< std::reference_wrapper<XML::bpmn::tResourceRole> >& resources) {
  for ( auto& resource : resources ) {
    if ( auto performer = resource.get().get<XML::bpmn::tPerformer>();
      performer && performer->name.has_value() && performer->name.value().get().value.value == "Sequential"
    ) {
      return true;
    } 
  }
  return false;
}



