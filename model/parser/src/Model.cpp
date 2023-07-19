#include "Model.h"
#include "DecisionTask.h"
#include "MessageTaskSubstitution.h"
#include <iostream>

using namespace BPMNOS;

Model::Model(const std::string& filename)
{
  readBPMNFile(filename);
}

std::unique_ptr<BPMN::Process> Model::createProcess(XML::bpmn::tProcess* process) {
  // bind status, restrictions, and operators to all processes
  return bind<BPMN::Process>(BPMN::Model::createProcess(process),std::make_unique<Status>(process));
}

std::unique_ptr<BPMN::FlowNode> Model::createActivity(XML::bpmn::tActivity* activity, BPMN::Scope* parent) {
  // bind status, restrictions, and operators to all activities
  auto node = bind<BPMN::FlowNode>(BPMN::Model::createActivity(activity, parent),std::make_unique<Status>(activity,parent));

  if ( auto jobShop = node->parent->represents<JobShop>(); jobShop ) {
    // add node to job list of resource activity
    jobShop->resourceActivity->jobs.push_back(node->as<BPMN::Activity>());
  }

  return node;
}

std::unique_ptr<BPMN::SequenceFlow> Model::createSequenceFlow(XML::bpmn::tSequenceFlow* sequenceFlow, BPMN::Scope* scope) {
  // bind gatekeeper restricitions to all sequence flows
  return bind<BPMN::SequenceFlow>(BPMN::Model::createSequenceFlow(sequenceFlow, scope),std::make_unique<Gatekeeper>(sequenceFlow));
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

// TODO
std::unique_ptr<BPMN::FlowNode> Model::createTask(XML::bpmn::tTask* task, BPMN::Scope* parent) {
  if ( const auto& type = task->getOptionalAttributeByName("type"); 
       type.has_value() && type->get().xmlns == "https://bpmn.telematique.eu/execution" 
  ) {
    if ( type->get().value == "Decision" ) {
      return std::make_unique<DecisionTask>(task,parent);
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
///std::cout << substitutionString(task,parent) << std::endl;
  }
  return BPMN::Model::createTask(task, parent);
}

