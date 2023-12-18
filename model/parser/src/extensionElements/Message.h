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
  ParameterMap parameterMap; ///< Map allowing to look up parameters by their names.
  std::set< std::string > header; ///< Set of parameter names
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.
  std::vector< BPMN::FlowNode* > candidates; ///< List of all potential senders or receivers of the message.
  Values getHeaderValues(Values& status) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Message_H
