#include <iostream>
#include <sstream>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <bpmn++.h>
#include "../parser/lib/bpmnos-model.h"

using namespace std;
using namespace BPMNOS;
//namespace xml = XML::bpmn; // use alias to shorten declarations

/*
// Optionally extend the model
class CustomNode : public Node {
public:
	CustomNode(XML::bpmn::tProcess& process) : Node(process) { x = 42; };
  CustomNode(XML::bpmn::tFlowNode& flowNode, Node* parentNode) : Node(flowNode,parentNode) { x = 23;};
  int x;
};

class CustomSequenceFlow : public SequenceFlow {
public:
	CustomSequenceFlow(XML::bpmn::tSequenceFlow& sequenceFlow, Node* scope) : SequenceFlow(sequenceFlow,scope) { y = 317; };
  int y;
};

class CustomModel : public Model {
public:
	CustomModel(const std::string& filename)  { Model::readBPMNFile(filename); };
  std::unique_ptr<Node> createRootNode(XML::bpmn::tProcess& process) override {
//cout << "CustomModel:createRootNode" << endl;
    return std::make_unique<CustomNode>(process);
  };
  std::unique_ptr<Node> createChildNode(XML::bpmn::tFlowNode& flowNode, Node* parentNode) override {
//cout << "CustomModel:createChildNode" << endl;
    return std::make_unique<CustomNode>(flowNode,parentNode);
  };
  std::unique_ptr<SequenceFlow> createSequenceFlow(XML::bpmn::tSequenceFlow& sequenceFlow, Node* scope) override {
//cout << "CustomModel:createSequenceFlow" << endl;
    return std::make_unique<CustomSequenceFlow>(sequenceFlow,scope);
  };
};
*/

int main(int argc, char **argv) {
  if ( argc != 2 ) {
    cout << "Usage: " << argv[0] << " <bpmn-file>" << endl;
    return 0;
  }

  BPMNOS::Model model(argv[1]); // Use basic model
//  CustomModel model(argv[1]); // Use extended model

  cout << "Number of processes: " << model.processes.size() << endl;
  for ( auto& processNode : model.processes ) {
//    if ( processNode->represents<CustomNode>() ) cout << "CustomNode" << endl;
//    cout << processNode->as<CustomNode>()->x << endl;

    XML::bpmn::tProcess& process = *processNode->get<XML::bpmn::tProcess>();

    cout << "- Process";
    if ( process.id.has_value() ) {
      cout << " with id '" << (string)process.id->get() << "'";
    }
    else {
      cout << " without id";
    }
    if ( process.isExecutable.has_value() && process.isExecutable->get() ) {
      cout << " is executable and"; 
    }

    cout << " has " << processNode->childNodes.size() << " child node(s)";  
    cout << " and " << processNode->sequenceFlows.size() << " sequence flow(s) and starts with:";
    for ( auto& startNode : processNode->startNodes ) {
      cout << " " << startNode->id;
    }
    cout << "." << endl;

    for ( auto& childNode : processNode->childNodes ) {
//    cout << childNode->as<CustomNode>()->x << endl;
      cout << "  - " << childNode->get<>()->className;
      if ( childNode->id.size() ) {
        cout << " with id '" << (string)childNode->id << "'";
      }
      else {
        cout << " without id";
      }
      cout << " has " << childNode->childNodes.size() << " child node(s)";  
      cout << ", " << childNode->incoming.size() << " incoming and " << childNode->outgoing.size() << " outgoing arc(s)." << endl;
      for ( auto& incoming : childNode->incoming ) {
//        cout << incoming->as<CustomSequenceFlow>()->y << endl;
        cout << "    - from node " << (std::string)incoming->source->id << endl;
      }
      for ( auto& outgoing : childNode->outgoing ) {
//        cout << outgoing->as<CustomSequenceFlow>()->y << endl;
        cout << "    - to node " << (std::string)outgoing->target->id << endl;
      }
    }

    for ( XML::bpmnos::tAttribute& attribute : processNode->as<Node>()->status ) {
        cout << "  - Attribute " << (std::string)attribute.getRequiredAttributeByName("id") << " has name " << (std::string)attribute.getRequiredAttributeByName("name") << endl;
    }

  }

  return 0;
}

