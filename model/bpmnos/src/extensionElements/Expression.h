#ifndef BPMNOS_Model_Expression_H
#define BPMNOS_Model_Expression_H

#include <exprtk.hpp>

#include "Attribute.h"
#include "AttributeRegistry.h"
#include "Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"

namespace BPMNOS::Model {

/**
 * @brief Abstract base class representing an operator that uses an expression to determine the value
 * of a status attribute.
 **/
class Expression {
public:
  Expression(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry);
  virtual ~Expression() = default;  // Virtual destructor
  const AttributeRegistry& attributeRegistry;
  const std::string expression;
  static std::unique_ptr<Expression> create(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry);
  std::set<const Attribute*> inputs; ///< Vector containing all input attributes influencing the result of the expression.

  virtual std::optional<BPMNOS::number> execute(const Values& status, const Values& data, const BPMNOS::Values& globals) const = 0;
  virtual std::optional<BPMNOS::number> execute(const Values& status, const SharedValues& data, const BPMNOS::Values& globals) const = 0;

  virtual std::pair< std::optional<BPMNOS::number>, std::optional<BPMNOS::number> > getBounds(const Attribute* attribute, const Values& status, const Values& data, const BPMNOS::Values& globals) const;
  virtual std::pair< std::optional<BPMNOS::number>, std::optional<BPMNOS::number> > getBounds(const Attribute* attribute, const Values& status, const SharedValues& data, const BPMNOS::Values& globals) const;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Expression_H
