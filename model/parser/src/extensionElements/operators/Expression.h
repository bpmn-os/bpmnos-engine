#ifndef BPMNOS_Expression_H
#define BPMNOS_Expression_H

#include <exprtk.hpp>

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/extensionElements/Operator.h"

namespace BPMNOS {

class Expression : public Operator {
public:
  Expression(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);

  static std::unique_ptr<Expression> create(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);
  virtual void apply(Values& values) const = 0;
};

} // namespace BPMNOS

#endif // BPMNOS_Expression_H
