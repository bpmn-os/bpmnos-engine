#include <unordered_set>

#include "Model.h"
#include "extensionElements/Status.h"
#include "extensionElements/Gatekeeper.h"
#include "extensionElements/Timer.h"
#include "extensionElements/MessageSender.h"
#include "extensionElements/MessageRecipient.h"

#include "DecisionTask.h"
#include "MessageTaskSubstitution.h"
#include "JobShop.h"
#include "ResourceActivity.h"
#include "RequestActivity.h"
#include "ReleaseActivity.h"

using namespace BPMNOS;

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
  // bind attributes, restrictions, and operators to all other activities
  node = bind<BPMN::FlowNode>( node, std::make_unique<Status>(activity,parent) );

  if ( auto jobShop = node->parent->represents<JobShop>(); jobShop ) {
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

std::unique_ptr<BPMN::FlowNode> Model::createMessageBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageBoundaryEvent(boundaryEvent,parent),
    std::make_unique<MessageRecipient>(boundaryEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageCatchEvent(catchEvent,parent),
    std::make_unique<MessageRecipient>(catchEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) {
  // bind message content
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageThrowEvent(throwEvent,parent),
    std::make_unique<MessageSender>(throwEvent,parent)
  );
}

void Model::createMessageFlows() {
  BPMN::Model::createMessageFlows();

  // Messages can only flow between message extensions with the same name.
  // If message flows are given in the model, these act as a restriction.

  for ( auto& sendingProcess : processes ) {
    // find all throwing message events of the sending process
    auto throwingMessageEvents = sendingProcess->find_all(
      [](const BPMN::Node* n) { return n->extensionElements->represents<MessageSender>();}
    );

    for ( auto throwingMessageEvent : throwingMessageEvents ) {
      // determine all allowed recipient for the throwing message event
      MessageSender* messageSender = throwingMessageEvent->extensionElements->as<MessageSender>();

      // determine all message flows in the model that leave the node or an ancestor
      std::unordered_set<BPMN::MessageFlow*> outflows;
      BPMN::Node* node = throwingMessageEvent;
      while ( auto flowNode = node->represents<BPMN::FlowNode>() ) {
        for ( auto& messageFlow : messageFlows ) {
          if ( messageFlow->source.second == flowNode ) {
            outflows.insert(messageFlow.get());
          }
        }
        node = flowNode->parent;
      }
      for ( auto& messageFlow : messageFlows ) {
        if ( auto process = node->represents<BPMN::Process>(); process && messageFlow->source.first == process ) {
          outflows.insert(messageFlow.get());
        }
      }

      for ( auto& receivingProcess : processes ) {
        // only consider node pairs belonging to different processes
        if ( sendingProcess.get() != receivingProcess.get() ) {
          // find all catching message events of receiving process which have the same message name
          auto catchingMessageEvents = receivingProcess->find_all(
            [&messageSender](const BPMN::Node* n) {
              if ( auto messageRecipient = n->extensionElements->represents<MessageRecipient>();
                   messageRecipient 
                   && messageSender->name == messageRecipient->name 
              ) {
                return true;
              }
              return false;
            }
          );

          for ( auto catchingMessageEvent : catchingMessageEvents ) {
            MessageRecipient* messageRecipient = catchingMessageEvent->extensionElements->as<MessageRecipient>();

            // determine all message flows in the model entering the node or an ancestor
            std::unordered_set<BPMN::MessageFlow*> inflows;
            BPMN::Node* node = catchingMessageEvent;
            while ( auto flowNode = node->represents<BPMN::FlowNode>() ) {
              for ( auto& messageFlow : messageFlows ) {
                if ( messageFlow->target.second == flowNode ) {
                  inflows.insert(messageFlow.get());
                }
              }
              node = flowNode->parent;
            }
            for ( auto& messageFlow : messageFlows ) {
              if ( auto process = node->represents<BPMN::Process>(); process && messageFlow->target.first == process ) {
                inflows.insert(messageFlow.get());
              }
            }

            // a message is only allowed if outflows and inflows are either empty or intersect 
            if ( 
              ( outflows.empty() && inflows.empty() ) || 
              std::any_of(outflows.begin(), outflows.end(), [&](BPMN::MessageFlow* messageFlow) { return inflows.count(messageFlow) > 0; })
            ) {
              // set allowedRecipients for each throwing message event
              messageSender->allowedRecipients.push_back(catchingMessageEvent->as<BPMN::CatchEvent>());

              // set allowedSenders for each catching message event
              messageRecipient->allowedSenders.push_back(throwingMessageEvent->as<BPMN::ThrowEvent>());
            }

          }
        }
      }
    }
  }
}


