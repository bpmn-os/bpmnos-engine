#include "Model.h"
#include "Status.h"
#include "Gatekeeper.h"

using namespace BPMNOS;

Model::Model(const std::string& filename)
{
  readBPMNFile(filename);
}

std::unique_ptr<BPMN::Process> Model::createProcess(XML::bpmn::tProcess* process) {
  return bind<BPMN::Process>(BPMN::Model::createProcess(process),std::make_unique<Status>(process));
}

std::unique_ptr<BPMN::FlowNode> Model::createActivity(XML::bpmn::tActivity* activity, BPMN::Scope* parent) {
  return bind<BPMN::FlowNode>(BPMN::Model::createActivity(activity, parent),std::make_unique<Status>(activity));
}

std::unique_ptr<BPMN::SequenceFlow> Model::createSequenceFlow(XML::bpmn::tSequenceFlow* sequenceFlow, BPMN::Scope* scope) {
  return bind<BPMN::SequenceFlow>(BPMN::Model::createSequenceFlow(sequenceFlow, scope),std::make_unique<Gatekeeper>(sequenceFlow));
}

/*
std::unique_ptr<BPMN::Node> Model::createChildNode(XML::bpmn::tFlowNode& flowNode, BPMN::Node* parentNode) {
std::cout << "BPMNOS::Model:createChildNode" << std::endl;
    if ( auto subProcess = flowNode.is<XML::bpmn::tSubProcess>(); subProcess ) {
      if ( const auto& type = subProcess->getOptionalAttributeByName("type"); 
           type.has_value() && type->get().xmlns == "https://bpmn.telematique.eu/resources" 
      ) {
        if ( (std::string)type->get() == "Resource" ) {
          return std::make_unique<ResourceActivity>(*subProcess,parentNode);
        }
        else if ( (std::string)type->get() == "Request" ) {
          return std::make_unique<RequestActivity>(*subProcess,parentNode);
        }
        else if ( (std::string)type->get() == "Release" ) {
          return std::make_unique<ReleaseActivity>(*subProcess,parentNode);
        }
      }
    }

    if ( auto activity = flowNode.is<XML::bpmn::tActivity>(); activity ) {
      if ( const auto& type = parentNode->element->getOptionalAttributeByName("type"); 
         type.has_value() 
         && type->get().xmlns == "https://bpmn.telematique.eu/resources" 
         && (std::string)type->get() == "JobShop" 
         && !activity->is<XML::bpmn::tSendTask>()
         && !activity->is<XML::bpmn::tReceiveTask>()
      ) {
        return std::make_unique<Job>(*activity,parentNode);
      }
    }

    return std::make_unique<Node>(flowNode,parentNode);
};
*/
