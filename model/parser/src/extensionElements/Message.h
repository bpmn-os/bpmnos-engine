#ifndef BPMNOS_Model_Message_H
#define BPMNOS_Model_Message_H

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Content.h"

namespace BPMNOS::Model {

class Message : public BPMN::ExtensionElements {
public:
  Message(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  std::string name; ///< Message name
  std::vector< std::unique_ptr<Parameter> > parameters;
  std::set< std::string > header; ///< Set of parameter names
//  std::optional< std::unique_ptr<Parameter> > request; ///< Optional reference to request associated to message.
  std::vector< std::unique_ptr<Content> > contents;
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.
  std::vector< BPMN::FlowNode* > candidates; ///< List of all potential senders or receivers of the message.

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Message_H
