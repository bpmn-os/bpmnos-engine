#ifndef BPMNOS_Model_Expression_H
#define BPMNOS_Model_Expression_H

#include <exprtk.hpp>

#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"

namespace BPMNOS::Model {

/**
 * @brief Abstract base class representing an operator that uses an expression to determine that value
 * of a status attribute.
 **/
class Expression {
public:
  Expression(XML::bpmnos::tParameter* parameter, const AttributeMap& statusAttributes);
  virtual ~Expression() = default;  // Virtual destructor
  const AttributeMap& statusAttributes;
  const std::string expression;
  static std::unique_ptr<Expression> create(XML::bpmnos::tParameter* parameter, const AttributeMap& statusAttributes);
  virtual std::optional<BPMNOS::number> execute(const Values& values) const = 0;
  virtual std::pair< std::optional<BPMNOS::number>, std::optional<BPMNOS::number> > getBounds(const Attribute* attribute, const Values& values) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Expression_H
