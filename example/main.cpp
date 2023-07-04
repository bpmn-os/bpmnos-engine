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

    cout << " has " << processNode->flowNodes.size() << " child node(s)";  
    cout << " and " << processNode->sequenceFlows.size() << " sequence flow(s) and starts with:";
    for ( auto& startNode : processNode->startNodes ) {
      cout << " " << startNode->id;
    }
    cout << "." << endl;

    for ( auto& flowNode : processNode->flowNodes ) {
      cout << "  - " << flowNode->get<>()->className;
      if ( flowNode->id.size() ) {
        cout << " with id '" << (string)flowNode->id << "'";
      }
      else {
        cout << " without id";
      }
      cout << " has ";
      if ( auto scope = flowNode->represents<BPMN::Scope>(); scope ) {
        cout << scope->flowNodes.size() << " child node(s), ";
      }
      cout << flowNode->incoming.size() << " incoming and " << flowNode->outgoing.size() << " outgoing arc(s)." << endl;
      for ( auto& incoming : flowNode->incoming ) {
        cout << "    - from node " << (std::string)incoming->source->id << endl;
      }
      for ( auto& outgoing : flowNode->outgoing ) {
        cout << "    - to node " << (std::string)outgoing->target->id << endl;
      }
    }
    for ( XML::bpmnos::tAttribute& attribute : processNode->extensionElements->as<Status>()->status ) {
        cout << "  - Attribute '" << (std::string)attribute.id << "' of type '" << (std::string)attribute.type << "' has name '" << (std::string)attribute.name << "'" << endl;
    }
  }

  return 0;
}

