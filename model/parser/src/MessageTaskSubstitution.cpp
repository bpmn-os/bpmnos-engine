#include "MessageTaskSubstitution.h"
#include <regex>
#include <string>
#include <algorithm>
#include "strutil.h"
#include "RequestActivity.h"
#include "ReleaseActivity.h"

using namespace BPMNOS;


MessageTaskSubstitution::MessageTaskSubstitution(std::unique_ptr<XML::XMLObject> substitutionRoot, BPMN::Scope* parent)
  : BPMN::Node(getSubProcess(substitutionRoot.get()))
  , BPMN::SubProcess(getSubProcess(substitutionRoot.get()),parent)
{
  root = std::move(substitutionRoot);
}


XML::bpmn::tSubProcess* MessageTaskSubstitution::getSubProcess(XML::XMLObject* substitutionRoot) {
  auto result = substitutionRoot->find<XML::bpmn::tSubProcess>();
  if ( result.size() != 1 ) {
    throw std::logic_error("MessageTaskSubstitution: substitution root must contain excatly one subprocess");
  }
  return &result[0].get();
}

#include <iostream>
std::unique_ptr<XML::XMLObject> MessageTaskSubstitution::substitute(XML::bpmn::tTask* task, BPMN::Scope* parent) {
  if ( !task->id.has_value() ) {
    throw std::runtime_error("MessageTaskSubstitution: task has no id");
  } 
  std::string id = task->id->get();

  std::string taskString = task->stringify();
  if ( task->prefix.empty() ) {
    throw std::runtime_error("MessageTaskSubstitution: cannot get XML namespace prefix for task " + id);
  }
  else if ( task->prefix != "bpmn2" ) {
    auto prefix = task->prefix + ":";
    strutil::replace_all(taskString, prefix, "bpmn2:");
  }

  // reinterpret task as subprocess
  taskString = std::regex_replace(taskString, std::regex("sendTask"), "subProcess"); 
  taskString = std::regex_replace(taskString, std::regex("receiveTask"), "subProcess"); 

  std::string xmlString = std::regex_replace(xmlTemplate, std::regex("<bpmn2:subProcess/>"), taskString); 

  // include parallized message events
  std::string contentString = subProcessTemplate;
  
  if ( !task->name.has_value() ) {
    throw std::runtime_error("MessageTaskSubstitution: task " + id + " has no name");
  } 
  std::string taskName = strutil::to_lower(task->name->get());
  taskName = std::regex_replace(taskName, std::regex("\n"), " "); 

  auto matchingMessage = [taskName](const auto& item) { 
    return strutil::contains(taskName,strutil::to_lower(item.get().name)); 
  };

  struct Message { std::string reference; std::string xml; };
  std::vector< Message > messages;
  
  std::vector< std::string > references;
  if ( auto requestActivity = parent->represents<RequestActivity>(); requestActivity ) {
    for ( auto request : requestActivity->requests ) {
      if ( auto message = std::find_if(request.get().message.begin(), request.get().message.end(), matchingMessage); message != request.get().message.end() ) {
        messages.push_back({request.get().id,message->get().stringify()});
      }
      else {
        throw std::runtime_error("MessageTaskSubstitution: cannot find '" + taskName + "' for task " + id );
      }
    }
  }
  else if ( auto releaseActivity = parent->represents<ReleaseActivity>(); releaseActivity ) {
    for ( auto release : releaseActivity->releases ) {
      if ( auto message = std::find_if(release.get().message.begin(), release.get().message.end(), matchingMessage); message != release.get().message.end() ) {
        messages.push_back({release.get().request,message->get().stringify()});
      }
      else {
        throw std::runtime_error("MessageTaskSubstitution: cannot find '" + taskName + "' for task " + id);
      }
    }
  }
  else {
    throw std::runtime_error("MessageTaskSubstitution: task " + id + "does not belong to request or release activity");
  }


  for ( auto message : messages ) {
    // add event for message
    std::string messageString = messageTemplate;

    if ( task->is<XML::bpmn::tSendTask>() ) {
      messageString = std::regex_replace(messageString, std::regex("%MESSAGEEVENT%"), "intermediateThrowEvent"); 
    }
    else {
      messageString = std::regex_replace(messageString, std::regex("%MESSAGEVENT%"), "intermediateCatchEvent"); 
    }
    messageString = std::regex_replace(messageString, std::regex("%REFERENCE%"), message.reference); 

    messageString = std::regex_replace(messageString, std::regex("<bpmn2:extensionElements/>"), "<bpmn2:extensionElements>" + message.xml + "</bpmn2:extensionElements>"); 

    contentString += messageString;

    std::string outgoing = std::regex_replace(outgoingTemplate, std::regex("%REFERENCE%"), message.reference); 
    contentString = std::regex_replace(contentString, std::regex("%OUTGOING%"), outgoing + "%OUTGOING%"); 

    std::string incoming = std::regex_replace(incomingTemplate, std::regex("%REFERENCE%"), message.reference); 
    contentString = std::regex_replace(contentString, std::regex("%INCOMING%"), incoming + "%INCOMING%"); 
  }
  contentString = std::regex_replace(contentString, std::regex("%OUTGOING%"), ""); 
  contentString = std::regex_replace(contentString, std::regex("%INCOMING%"), ""); 

  contentString = std::regex_replace(contentString, std::regex("%ID%"), id); 

  xmlString = std::regex_replace(xmlString, std::regex("</bpmn2:subProcess>"), contentString + "</bpmn2:subProcess>"); 

  // obtain objects
  std::unique_ptr<XML::XMLObject> root(XML::XMLObject::createFromString(xmlString));
  auto& subProcess = root->getRequiredChild<XML::bpmn::tSubProcess>();

  // remove loop characteristics
  std::erase_if(subProcess.children, [](const std::unique_ptr<XML::XMLObject>& child){ return (child->is<XML::bpmn::tLoopCharacteristics>() != nullptr); } );

  return std::unique_ptr<XML::XMLObject>(XML::XMLObject::createFromString(xmlString));
}

