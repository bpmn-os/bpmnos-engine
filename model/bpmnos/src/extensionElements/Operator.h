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
  Operator(XML::bpmnos::tOperator* operator_, const AttributeMap& statusAttributes);
  virtual ~Operator() = default;  // Virtual destructor
  XML::bpmnos::tOperator* element;

  std::string& id;
  Attribute* attribute;
  const AttributeMap& statusAttributes;
  ParameterMap parameterMap;

  static std::unique_ptr<Operator> create(XML::bpmnos::tOperator* operator_, AttributeMap& statusAttributes);
  virtual void apply(Values& values) const = 0;

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