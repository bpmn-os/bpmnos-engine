#ifndef BPMNOS_Model_MessageTaskSubstitution_H
#define BPMNOS_Model_MessageTaskSubstitution_H

#include <bpmn++.h>
#include <string>
#include "xml/bpmnos/tMessage.h"

namespace BPMNOS::Model {

class MessageTaskSubstitution : public BPMN::SubProcess {
  friend class Model;
public:
  MessageTaskSubstitution(std::unique_ptr<XML::XMLObject> substitutionRoot, BPMN::Scope* parent);
  static std::unique_ptr<XML::XMLObject> substitute(XML::bpmn::tTask* task, BPMN::Scope* parent);
protected:
  std::unique_ptr<XML::XMLObject> root;
  static XML::bpmn::tSubProcess* getSubProcess(XML::XMLObject* substitutionRoot);

  static inline const std::string xmlTemplate = "<bpmn2:definitions xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:bpmn2=\"http://www.omg.org/spec/BPMN/20100524/MODEL\" xmlns:execution=\"https://bpmn.telematique.eu/execution\" targetNamespace=\"http://bpmn.io/schema/bpmn\"><bpmn2:subProcess/></bpmn2:definitions>";
  static inline const std::string subProcessTemplate =
     "<bpmn2:startEvent id=\"StartEvent_%ID%\"><bpmn2:outgoing>Flow_StartFork_%ID%</bpmn2:outgoing></bpmn2:startEvent>"
    "<bpmn2:parallelGateway id=\"ForkGateway_%ID%\"><bpmn2:incoming>Flow_StartFork_%ID%</bpmn2:incoming>%OUTGOING%</bpmn2:parallelGateway>"
    "<bpmn2:endEvent id=\"EndEvent_%ID%\"><bpmn2:incoming>Flow_JoinEnd_%ID%</bpmn2:incoming></bpmn2:endEvent>"
    "<bpmn2:parallelGateway id=\"JoinGateway_%ID%\">%INCOMING%<bpmn2:outgoing>Flow_JoinEnd_%ID%</bpmn2:outgoing></bpmn2:parallelGateway>"
    "<bpmn2:sequenceFlow id=\"Flow_StartFork_%ID%\" sourceRef=\"StartEvent_%ID%\" targetRef=\"ForkGateway_%ID%\" />"
    "<bpmn2:sequenceFlow id=\"Flow_JoinEnd_%ID%\" sourceRef=\"JoinGateway_%ID%\" targetRef=\"EndEvent_%ID%\" />";

  static inline const std::string messageTemplate = 
    "<bpmn2:%MESSAGEEVENT% id=\"MessageEvent_%ID%_%REFERENCE%\"><bpmn2:incoming>Flow_ForkEvent_%ID%_%REFERENCE%</bpmn2:incoming><bpmn2:outgoing>Flow_EventJoin_%ID%_%REFERENCE%</bpmn2:outgoing><bpmn2:messageEventDefinition id=\"MessageEventDefinition_%ID%_%REFERENCE%\" /><bpmn2:extensionElements/></bpmn2:%MESSAGEEVENT%>"
    "<bpmn2:sequenceFlow id=\"Flow_ForkEvent_%ID%_%REFERENCE%\" sourceRef=\"ForkGateway_%ID%\" targetRef=\"MessageEvent_%ID%_%REFERENCE%\" />"
    "<bpmn2:sequenceFlow id=\"Flow_EventJoin_%ID%_%REFERENCE%\" sourceRef=\"MessageEvent_%ID%_%REFERENCE%\" targetRef=\"JoinGateway_%ID%\" />";

  static inline const std::string outgoingTemplate = "<bpmn2:outgoing>Flow_ForkEvent_%ID%_%REFERENCE%</bpmn2:outgoing>";
  static inline const std::string incomingTemplate = "<bpmn2:incoming>Flow_EventJoin_%IDENTIFIER_%REFERENCE%</bpmn2:incoming>";

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_MessageTaskSubstitution_H

