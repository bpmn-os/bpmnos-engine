#ifndef BPMNOS_Operator_H
#define BPMNOS_Operator_H

#include <memory>
#include <optional>
#include <vector>
#include <string>
#include <bpmn++.h>
#include <variant>
#include "model/utility/src/Numeric.h"
#include "model/utility/src/StringRegistry.h"
#include "model/parser/src/xml/bpmnos/tOperator.h"
#include "Attribute.h"
#include "Parameter.h"
#include "operators/Unset.h"
#include "operators/Set.h"
#include "operators/Lookup.h"
#include "operators/Expression.h"

namespace BPMNOS {

class Operator {
public:
  Operator(XML::bpmnos::tOperator* operator_, AttributeMap& attributeMap);
  XML::bpmnos::tOperator* element;

  std::string& id;
  Attribute* attribute;
  const AttributeMap& attributeMap;
  ParameterMap parameterMap;

  std::variant< std::unique_ptr<Set>, std::unique_ptr<Unset>, std::unique_ptr<Lookup>, std::unique_ptr<Expression> > implementation;

  // Use std::variant instead because virtual template method cannot be used 
  template <typename T>
  void apply(std::vector<std::optional<T> >& values) const {
    if ( auto operatorImplementation = std::get_if< std::unique_ptr<Set> >(&implementation); operatorImplementation ) {
      operatorImplementation->get()->execute(values);
    }
    else if ( auto operatorImplementation = std::get_if< std::unique_ptr<Unset> >(&implementation); operatorImplementation ) {
      operatorImplementation->get()->execute(values);
    }
    else if ( auto operatorImplementation = std::get_if< std::unique_ptr<Lookup> >(&implementation); operatorImplementation ) {
      operatorImplementation->get()->execute(values);
    }
    else if ( auto operatorImplementation = std::get_if< std::unique_ptr<Expression> >(&implementation); operatorImplementation ) {
      operatorImplementation->get()->execute(values);
    }
    else {
      throw std::logic_error("Operator: illegal alternative for operator '" + id + "'");
    }
  }
};

} // namespace BPMNOS

#endif // BPMNOS_Operator_H
