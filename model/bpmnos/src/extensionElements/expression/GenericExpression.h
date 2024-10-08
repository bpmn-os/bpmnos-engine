#ifndef BPMNOS_Model_GenericExpression_H
#define BPMNOS_Model_GenericExpression_H

#include <exprtk.hpp>

#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/bpmnos/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/bpmnos/src/extensionElements/Expression.h"

namespace BPMNOS::Model {


/**
 * @brief Class representing an operator that uses a generic expression to determine that value
 * of a status attribute.
 **/
class GenericExpression : public Expression {
public:
  GenericExpression(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry);

  // From: https://www.partow.net/programming/exprtk/index.html:
  // Note: NumericType can be any floating point type. This includes but is not limited to:
  // float, double, long double, MPFR or any custom type conforming to an interface compatible
  // with the standard floating point type.
  using NumericType = double;

  exprtk::expression<NumericType> compiledExpression; 
  std::vector< std::pair<NumericType&, Attribute *> > bindings; ///< Bindings of expression variables. 

  template <typename DataType>
  std::optional<BPMNOS::number> _execute(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;

  std::optional<BPMNOS::number> execute(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const override { return _execute(status,data,globals); };
  std::optional<BPMNOS::number> execute(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const override { return _execute(status,data,globals); };
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_GenericExpression_H
