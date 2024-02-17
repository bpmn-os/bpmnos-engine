#ifndef BPMNOS_Model_Expression_H
#define BPMNOS_Model_Expression_H

#include <exprtk.hpp>

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/extensionElements/Operator.h"

namespace BPMNOS::Model {

/**
 * @brief Abstract base class representing an operator that uses an expression to determine that value
 * of a status attribute.
 **/
class Expression : public Operator {
public:
  Expression(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);
  virtual ~Expression() = default;  // Virtual destructor

  static std::unique_ptr<Expression> create(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);
  virtual void apply(Values& values) const = 0;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Expression_H
