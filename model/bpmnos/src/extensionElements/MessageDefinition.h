#ifndef BPMNOS_Model_MessageDefinition_H
#define BPMNOS_Model_MessageDefinition_H

#include <memory>
#include <vector>
#include <set>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Content.h"
#include "model/bpmnos/src/xml/bpmnos/tMessage.h"

namespace BPMNOS::Model {

class MessageDefinition {
public:
  MessageDefinition(XML::bpmnos::tMessage* message, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tMessage* element;
  BPMNOS::number name; ///< Message name
  ParameterMap parameterMap; ///< Map allowing to look up parameters by their names.
  std::vector< std::string > header; ///< Set of parameter names always beginning with "sender" and "recipient"
  enum Index { Name, Sender, Recipient };
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.

  template <typename DataType>
  BPMNOS::Values getSenderHeader(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data) const; /// Returns a vector of values including message name, recipient, sender, and all other header parameters

  template <typename DataType>
  BPMNOS::Values getRecipientHeader(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data) const; /// Returns a vector of values including message name, recipient, sender, and all other header parameters
private:

  template <typename DataType>
  std::optional<BPMNOS::number> getHeaderValue(const std::string& key, const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data ) const; ///< Returns the header value string with the given key represented as number.

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_MessageDefinition_H
