#ifndef BPMNOS_Model_Operator_H
#define BPMNOS_Model_Operator_H

#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <bpmn++.h>
#include <variant>
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/xml/bpmnos/tOperator.h"
#include "Attribute.h"
#include "Parameter.h"

namespace BPMNOS::Model {

/**
 * @brief Abstract base class for all operators allowing to manipulate attribute values.
 **/
class Operator {
public:
  Operator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry);
  virtual ~Operator() = default;  // Virtual destructor
  XML::bpmnos::tOperator* element;

  std::string& id;
  Attribute* attribute; ///< The status or data attribute to be modified by the operator
  const AttributeRegistry& attributeRegistry;
  ParameterMap parameterMap;

  static std::unique_ptr<Operator> create(XML::bpmnos::tOperator* operator_, AttributeRegistry& attributeRegistry);
  virtual void apply(Values& status, Globals& data) const = 0;
  virtual void apply(Values& status, Values& data) const = 0;

  /// Returns a pointer of type T of the node.
  template<typename T> T* is() {
    return dynamic_cast<T*>(this);
  }

  /// Returns a pointer of type T of the node.
  template<typename T> const T* is() const {
    return dynamic_cast<const T*>(this);
  }
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Operator_H
