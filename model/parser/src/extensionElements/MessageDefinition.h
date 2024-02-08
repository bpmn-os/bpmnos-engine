#ifndef BPMNOS_Model_MessageDefinition_H
#define BPMNOS_Model_MessageDefinition_H

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Content.h"
#include "model/parser/src/xml/bpmnos/tMessage.h"

namespace BPMNOS::Model {

class MessageDefinition {
public:
  MessageDefinition(XML::bpmnos::tMessage* message, AttributeMap& attributeMap);
  XML::bpmnos::tMessage* element;
  std::string& name; ///< Message name
  ParameterMap parameterMap; ///< Map allowing to look up parameters by their names.
  std::vector< std::string > header; ///< Set of parameter names always beginning with "sender" and "recipient"
  enum Index { Name, Sender, Recipient };
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.
  BPMNOS::Values getSenderHeader(const BPMNOS::Values& status) const; /// Returns a vector of values including message name, recipient, sender, and all other header parameters
  BPMNOS::Values getRecipientHeader(const BPMNOS::Values& status) const; /// Returns a vector of values including message name, recipient, sender, and all other header parameters
private:
  std::optional<BPMNOS::number> getHeaderValue(const BPMNOS::Values& status, const std::string& key) const; ///< Returns the header value string with the given key represented as number.

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_MessageDefinition_H
