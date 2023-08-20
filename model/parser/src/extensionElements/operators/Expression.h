#ifndef BPMNOS_Expression_H
#define BPMNOS_Expression_H

#include <exprtk.hpp>

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"

namespace BPMNOS {

class Operator;

class Expression {
public:
  Expression(Operator* base, Attribute* attribute);
  const Operator* base;
  Attribute* attribute;
  Parameter* parameter;

  // From: https://www.partow.net/programming/exprtk/index.html:
  // Note: NumericType can be any floating point type. This includes but is not limited to:
  // float, double, long double, MPFR or any custom type conforming to an interface compatible
  // with the standard floating point type.
  using NumericType = double;

  exprtk::expression<NumericType> expression; 
  std::vector< std::pair<NumericType&, Attribute *> > bindings; ///< Bindings of expression variables. 

  void execute(Values& status) const;
};

} // namespace BPMNOS

#endif // BPMNOS_Expression_H
