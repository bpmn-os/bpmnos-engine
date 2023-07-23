#include "Model.h"

#include "extensionElements/Status.h"
#include "extensionElements/Gatekeeper.h"
#include "extensionElements/Timer.h"
#include "extensionElements/Message.h"
#include "extensionElements/Decisions.h"

#include "DecisionTask.h"
#include "MessageTaskSubstitution.h"
#include "JobShop.h"
#include "ResourceActivity.h"
#include "RequestActivity.h"
#include "ReleaseActivity.h"

#include <iostream>

using namespace BPMNOS;

Model::Model(const std::string& filename)
{
  readBPMNFile(filename);
}

std::unique_ptr<BPMN::Process> Model::createProcess(XML::bpmn::tProcess* process) {
  // bind status, restrictions, and operators to all processes
  return bind<BPMN::Process>(
    BPMN::Model::createProcess(process),
    std::make_unique<Status>(process)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createActivity(XML::bpmn::tActivity* activity, BPMN::Scope* parent) {
  // bind status, restrictions, and operators to all activities
  auto node = bind<BPMN::FlowNode>(
    BPMN::Model::createActivity(activity, parent),
    std::make_unique<Status>(activity,parent)
  );

  if ( auto jobShop = node->parent->represents<JobShop>(); jobShop ) {
    // add node to job list of resource activity
    jobShop->resourceActivity->jobs.push_back(node->as<BPMN::Activity>());
  }

  return node;
}

std::unique_ptr<BPMN::SequenceFlow> Model::createSequenceFlow(XML::bpmn::tSequenceFlow* sequenceFlow, BPMN::Scope* scope) {
  // bind gatekeeper restricitions to all sequence flows
  return bind<BPMN::SequenceFlow>(
    BPMN::Model::createSequenceFlow(sequenceFlow, scope),
    std::make_unique<Gatekeeper>(sequenceFlow,scope)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createSubProcess(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent) {
  if ( const auto& type = subProcess->getOptionalAttributeByName("type"); 
       type.has_value() && type->get().xmlns == "https://bpmn.telematique.eu/resources" 
  ) {
    if ( type->get().value == "JobShop" ) {
      return std::make_unique<JobShop>(subProcess,parent);
    }
    else if ( type->get().value == "Resource" ) {
      return std::make_unique<ResourceActivity>(subProcess,parent);
    }
    else if ( type->get().value == "Request" ) {
      return std::make_unique<RequestActivity>(subProcess,parent);
    }
    else if ( type->get().value == "Release" ) {
      return std::make_unique<ReleaseActivity>(subProcess,parent);
    }
    else {
      throw std::runtime_error("Model: Illegal type '" + (std::string)type->get() + "'");
    }   
  }
  // return regular subProcess in all other cases
  return BPMN::Model::createSubProcess(subProcess, parent);
}

std::unique_ptr<BPMN::FlowNode> Model::createTask(XML::bpmn::tTask* task, BPMN::Scope* parent) {
  if ( const auto& type = task->getOptionalAttributeByName("type"); 
       type.has_value() && type->get().xmlns == "https://bpmn.telematique.eu/execution" 
  ) {
    if ( type->get().value == "Decision" ) {
      return bind<BPMN::FlowNode>(
        std::make_unique<DecisionTask>(task,parent),
        std::make_unique<Decisions>(task,parent)
      );
    }
    else {
      throw std::runtime_error("Model: Illegal type '" + (std::string)type->get() + "'");
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
  return bind<BPMN::FlowNode>(
    BPMN::Model::createTimerBoundaryEvent(boundaryEvent,parent),
    std::make_unique<Timer>(boundaryEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createTimerCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  return bind<BPMN::FlowNode>(
    BPMN::Model::createTimerCatchEvent(catchEvent,parent),
    std::make_unique<Timer>(catchEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageBoundaryEvent(XML::bpmn::tBoundaryEvent* boundaryEvent, BPMN::Scope* parent) {
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageBoundaryEvent(boundaryEvent,parent),
    std::make_unique<Message>(boundaryEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageCatchEvent(XML::bpmn::tCatchEvent* catchEvent, BPMN::Scope* parent) {
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageCatchEvent(catchEvent,parent),
    std::make_unique<Message>(catchEvent,parent)
  );
}

std::unique_ptr<BPMN::FlowNode> Model::createMessageThrowEvent(XML::bpmn::tThrowEvent* throwEvent, BPMN::Scope* parent) {
  return bind<BPMN::FlowNode>(
    BPMN::Model::createMessageThrowEvent(throwEvent,parent),
    std::make_unique<Message>(throwEvent,parent)
  );
}


