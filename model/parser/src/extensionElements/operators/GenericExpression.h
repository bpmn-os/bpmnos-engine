#ifndef BPMNOS_Model_GenericExpression_H
#define BPMNOS_Model_GenericExpression_H

#include <exprtk.hpp>

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "Expression.h"

namespace BPMNOS::Model {


class GenericExpression : public Expression {
public:
  GenericExpression(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);
  Parameter* parameter;

  // From: https://www.partow.net/programming/exprtk/index.html:
  // Note: NumericType can be any floating point type. This includes but is not limited to:
  // float, double, long double, MPFR or any custom type conforming to an interface compatible
  // with the standard floating point type.
  using NumericType = double;

  exprtk::expression<NumericType> expression; 
  std::vector< std::pair<NumericType&, Attribute *> > bindings; ///< Bindings of expression variables. 

  void apply(Values& status) const override;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_GenericExpression_H
