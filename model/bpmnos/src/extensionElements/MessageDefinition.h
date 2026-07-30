#ifndef BPMNOS_Model_MessageDefinition_H
#define BPMNOS_Model_MessageDefinition_H

#include <memory>
#include <optional>
#include <vector>
#include <set>
#include <string>
#include <bpmn++.h>
#include "Parameter.h"
#include "Content.h"
#include "model/utility/src/Value.h"
#include "model/bpmnos/src/xml/bpmnos/tMessage.h"

namespace BPMNOS::Model {

class MessageDefinition {
public:
  MessageDefinition(XML::bpmnos::tMessage* message, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tMessage* element;
  BPMNOS::number name; ///< Message name
  ParameterMap parameterMap; ///< Map allowing to look up parameters by their names.

  /**
   * @brief A key of the header together with the type of the values held under it.
   *
   * The type is that of the attribute a parameter names, or `STRING` where a parameter states a quoted
   * literal, and it is known when the model is read because a header parameter states nothing else. A
   * parameter without a value states no type, as it states no value, and the entry then holds no value
   * and is matched by any value held under the same key.
   */
  struct HeaderKey {
    std::string key;
    std::optional<ValueType> type;

    /// Two keys are the same where they are named alike and, wherever both state a type, hold values of
    /// that same type. A key stating no type is the same as a key stating any, its entry holding no
    /// value and being matched by every value held under that key.
    bool operator==(const HeaderKey& other) const {
      return key == other.key
        && ( !type.has_value() || !other.type.has_value() || type.value() == other.type.value() );
    }
  };

  std::vector< HeaderKey > header; ///< Keys of the header always beginning with "name", "sender", and "recipient"
  enum Index { Name, Sender, Recipient };
  ContentMap contentMap; ///< Map allowing to look up contents by their keys.

  template <typename DataType>
  BPMNOS::Values getSenderHeader(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const; /// Returns a vector of values including message name, recipient, sender, and all other header parameters

  template <typename DataType>
  BPMNOS::Values getRecipientHeader(const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const; /// Returns a vector of values including message name, recipient, sender, and all other header parameters
private:

  template <typename DataType>
  std::optional<BPMNOS::number> getHeaderValue(const std::string& key, const AttributeRegistry& attributeRegistry, const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& global ) const; ///< Returns the header value string with the given key represented as number.

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_MessageDefinition_H
