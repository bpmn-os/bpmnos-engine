#ifndef BPMNOS_Model_Operator_H
#define BPMNOS_Model_Operator_H

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <bpmn++.h>
#include <variant>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/xml/bpmnos/tOperator.h"
#include "Attribute.h"
#include "Expression.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing an operator allowing to manipulate attribute values.
 **/
class Operator {
public:
  Operator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tOperator* element;

  std::string& id;
  const Expression expression;
  const AttributeRegistry& attributeRegistry;
  Attribute* attribute; ///< The status or data attribute to be modified by the operator
  const std::set<const Attribute*>& inputs; ///< Set containing all input attributes influencing the result of the operator.

  template <typename DataType>
  void apply(BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const;

  /// Returns a pointer of type T of the node.
  template<typename T> T* is() {
    return dynamic_cast<T*>(this);
  }

  /// Returns a pointer of type T of the node.
  template<typename T> const T* is() const {
    return dynamic_cast<const T*>(this);
  }
private:
  Attribute* getAttribute() const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Operator_H
