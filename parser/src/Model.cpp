#include "Model.h"
using namespace BPMNOS;
#include <iostream>
Model::Model(const std::string& filename)
{
  readBPMNFile(filename);
}

std::unique_ptr<BPMN::Node> Model::createRootNode(XML::bpmn::tProcess& process) {
std::cout << "BPMNOS::Model:createRootNode" << std::endl;
    return std::make_unique<Node>(process);
  };
  std::unique_ptr<BPMN::Node> Model::createChildNode(XML::bpmn::tFlowNode& flowNode, BPMN::Node* parentNode) {
std::cout << "BPMNOS::Model:createChildNode" << std::endl;
    return std::make_unique<Node>(flowNode,parentNode);
  };
  std::unique_ptr<BPMN::SequenceFlow> Model::createSequenceFlow(XML::bpmn::tSequenceFlow& sequenceFlow, BPMN::Node* scope) {
std::cout << "BPMNOS::Model:createSequenceFlow" << std::endl;
    return std::make_unique<SequenceFlow>(sequenceFlow,scope);
  };

