#include <iostream>
#include <sstream>

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/util/PlatformUtils.hpp>

#include <bpmn++.h>
#include "../parser/lib/bpmnos-model.h"

using namespace std;
using namespace BPMNOS;

int main(int argc, char **argv) {
  if ( argc != 2 ) {
    cout << "Usage: " << argv[0] << " <bpmn-file>" << endl;
    return 0;
  }

  BPMNOS::Model model(argv[1]); // Use basic model

  cout << "Number of processes: " << model.processes.size() << endl;
  for ( auto& processNode : model.processes ) {

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
        cout << "    - from node " << (std::string)incoming->source->id << endl;
      }
      for ( auto& outgoing : childNode->outgoing ) {
        cout << "    - to node " << (std::string)outgoing->target->id << endl;
      }
    }

    for ( XML::bpmnos::tAttribute& attribute : processNode->as<Node>()->status ) {
        cout << "  - Attribute '" << (std::string)attribute.getRequiredAttributeByName("id") << "' of type '" << (std::string)attribute.getRequiredAttributeByName("type") << "' has name '" << (std::string)attribute.getRequiredAttributeByName("name") << "'" << endl;
    }

  }

  return 0;
}

