#include <unordered_set>
#include <cassert>

#include "Model.h"
#include "extensionElements/Status.h"
#include "extensionElements/MessageStatus.h"
#include "extensionElements/Gatekeeper.h"
#include "extensionElements/Timer.h"
#include "extensionElements/Message.h"
#include "DecisionTask.h"
#include "MessageTaskSubstitution.h"
#include "JobShop.h"
#include "ResourceActivity.h"
#include "RequestActivity.h"
#include "ReleaseActivity.h"

using namespace BPMNOS::Model;

Model::Model(const std::string& filename)
{
  readBPMNFile(filename);
}

std::unique_ptr<BPMN::Process> Model::createProcess(XML::bpmn::tProcess* process) {
  // bind attributes, restrictions, and operators to all processes
  return bind<BPMN::Process>(
    BPMN::Model::createProcess(process),
    std::make_unique<Status>(process)
  );
}

std::unique_ptr<BPMN::EventSubProcess> Model::createEventSubProcess(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent) {
  // bind attributes, restrictions, and operators to all processes
  return bind<BPMN::EventSubProcess>(
    BPMN::Model::createEventSubProcess(subProcess,parent),
    std::make_unique<Status>(subProcess)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createActivity(XML::bpmn::tActivity* activity, BPMN::Scope* parent) {
  auto node = BPMN::Model::createActivity(activity, parent);
  // bind attributes, restrictions, and operators to all activities
  if ( activity->is<XML::bpmn::tSendTask>() || activity->is<XML::bpmn::tReceiveTask>() ) {
    node = bind<BPMN::FlowNode>( node, std::make_unique<MessageStatus>(activity,parent) );
  }
  else {
    node = bind<BPMN::FlowNode>( node, std::make_unique<Status>(activity,parent) );
  }

  if ( auto jobShop = node->parent->represents<JobShop>() ) {
    // add node to job list of resource activity
    jobShop->resourceActivity->jobs.push_back(node->as<BPMN::Activity>());
   }

  return node;
}

std::unique_ptr<BPMN::SequenceFlow> Model::createSequenceFlow(XML::bpmn::tSequenceFlow* sequenceFlow, BPMN::Scope* scope) {
  // bind gatekeeper restrictions to all sequence flows
  return bind<BPMN::SequenceFlow>(
    BPMN::Model::createSequenceFlow(sequenceFlow, scope),
    std::make_unique<Gatekeeper>(sequenceFlow,scope)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createSubProcess(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent) {
  if ( const auto& type = subProcess->getOptionalAttributeByName("type"); 
       type.has_value() && type->get().xmlns == "https://bpmn.telematique.eu/resources" 
  ) {
    if ( type->get().value.value == "JobShop" ) {
      return std::make_unique<JobShop>(subProcess,parent);
    }
    else if ( type->get().value.value == "Resource" ) {
      return std::make_unique<ResourceActivity>(subProcess,parent);
    }
    else if ( type->get().value.value == "Request" ) {
      return std::make_unique<RequestActivity>(subProcess,parent);
    }
    else if ( type->get().value.value == "Release" ) {
      return std::make_unique<ReleaseActivity>(subProcess,parent);
    }
    else {
      throw std::runtime_error("Model: Illegal type '" + (std::string)type->get().value + "'");
    }   
  }
  // return regular subProcess in all other cases
  return BPMN::Model::createSubProcess(subProcess, parent);
}

std::unique_ptr<BPMN::FlowNode> Model::createTask(XML::bpmn::tTask* task, BPMN::Scope* parent) {
  if ( const auto& type = task->getOptionalAttributeByName("type"); 
       type.has_value() && type->get().xmlns == "https://bpmn.telematique.eu/execution" 
  ) {
    if ( type->get().value.value == "Decision" ) {
      // decisions are added with status
      return std::make_unique<DecisionTask>(task,parent);
    }
    else {
      throw std::runtime_error("Model: Illegal type '" + (std::string)type->get().value + "'");
    }   
  }
  else if ( ( parent->represents<RequestActivity>() || parent->represents<ReleaseActivity>() ) 
       && ( task->is<XML::bpmn::tSendTask>() || task->is<XML::bpmn::tReceiveTask>() ) 
  ) {
    // replace SendTask and ReceiveTask belonging to RequestActivity or ReleaseActivity by subprocess 
    // with parallel message events for each request or release
    return std::make_unique<MessageTaskSubstitution>(MessageTaskSubstitution::substitute(task,parent),parent);
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
    std::make_unique<Message>(startEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageBoundaryEvent(boundaryEvent,parent),
    std::make_unique<Message>(boundaryEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageCatchEvent(catchEvent,parent),
    std::make_unique<Message>(catchEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageThrowEvent(throwEvent,parent),
    std::make_unique<Message>(throwEvent,parent)
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

void Model::createMessageCandidates( BPMN::Process* sendingProcess, BPMN::FlowNode* throwingMessageEvent, BPMN::Process* receivingProcess, BPMN::FlowNode* catchingMessageEvent ) {

  auto sentMessage = throwingMessageEvent->extensionElements->represents<Message>();
  auto receivedMessage = catchingMessageEvent->extensionElements->represents<Message>();

  if (  throwingMessageEvent->represents<BPMN::SendTask>() ) {
    sentMessage = &throwingMessageEvent->extensionElements->represents<MessageStatus>()->message;
  }
  if (  catchingMessageEvent->represents<BPMN::ReceiveTask>() ) {
    receivedMessage = &throwingMessageEvent->extensionElements->represents<MessageStatus>()->message;
  }
 
  assert( sentMessage );
  assert( receivedMessage );

  if ( sentMessage->name != receivedMessage->name ) {
    return;
  }
  if ( sentMessage->header != receivedMessage->header ) {
    return;
  }

  // determine relevant message flows for throwing events
  auto& outgoingMessageFlows = determineMessageFlows(
    throwingMessageEvent, 
    [](BPMN::Node* node) -> std::vector<BPMN::MessageFlow*>& {
         return node->sending;
    }
  );

  if ( outgoingMessageFlows.size() ) {
    // determine whether catching message event is in message flow target
    bool found = false;
    for ( auto messageFlow : outgoingMessageFlows ) {
      auto& [process,flowNode] = messageFlow->target;
      if ( process == receivingProcess ) {
        if ( flowNode == catchingMessageEvent ) {
          found = true;
          break;
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

          if ( found ) {
            break;
          }
        }
      }
    }
    if ( !found ) {
      return;
    }
  }

  // determine relevant message flows for catching event
  auto incomingMessageFlows = determineMessageFlows(
    catchingMessageEvent, 
    [](BPMN::Node* node) -> std::vector<BPMN::MessageFlow*>& {
        return node->receiving;
    }
  );

  if ( incomingMessageFlows.size() ) {
    // determine whether throwing message event is in message flow source
    bool found = false;
    for ( auto messageFlow : incomingMessageFlows ) {
      auto& [process,flowNode] = messageFlow->source;
      if ( process == sendingProcess ) {
        if ( flowNode == throwingMessageEvent ) {
          found = true;
          break;
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
            break;
          }
        }
      }
    }
    if ( !found ) {
      return;
    }
  }
  
  // add message events to collection of candidates of each other
  sentMessage->candidates.push_back(catchingMessageEvent->as<BPMN::FlowNode>());
  receivedMessage->candidates.push_back(throwingMessageEvent->as<BPMN::FlowNode>());
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
