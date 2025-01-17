#ifndef BPMNOS_Model_Parameter_H
#define BPMNOS_Model_Parameter_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <variant>
#include <bpmn++.h>
#include "model/bpmnos/src/xml/bpmnos/tParameter.h"
#include "Attribute.h"
#include "Expression.h"
#include "AttributeRegistry.h"

namespace BPMNOS::Model {

class Parameter {
public:
  Parameter(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tParameter* element;

  std::string& name;
  std::unique_ptr<const Expression> expression;
/*
  std::optional< std::reference_wrapper<Attribute> > attribute;
  std::optional< std::reference_wrapper<XML::Value> > value;
*/
protected:
  std::unique_ptr<const Expression> getExpression(XML::bpmnos::tParameter* parameter, const AttributeRegistry& attributeRegistry) const;
//  std::optional< std::reference_wrapper<Attribute> > getAttribute(const AttributeRegistry& attributeRegistry) const;
};

typedef std::unordered_map< std::string, std::unique_ptr<Parameter> > ParameterMap;

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Parameter_H
