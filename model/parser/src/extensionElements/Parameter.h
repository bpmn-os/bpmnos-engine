#ifndef BPMNOS_Model_Parameter_H
#define BPMNOS_Model_Parameter_H

#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <optional>
#include <variant>
#include <bpmn++.h>
#include "model/parser/src/xml/bpmnos/tParameter.h"
#include "Attribute.h"

namespace BPMNOS::Model {

class Parameter {
public:
  Parameter(XML::bpmnos::tParameter* parameter, AttributeMap& attributeMap);
  XML::bpmnos::tParameter* element;

  std::string& name;
  std::optional< std::reference_wrapper<Attribute> > attribute;
  std::optional< std::reference_wrapper<XML::Value> > value;

protected:
  std::optional< std::reference_wrapper<Attribute> > getAttribute(AttributeMap& attributeMap);
};

typedef std::unordered_map< std::string, std::unique_ptr<Parameter> > ParameterMap;

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Parameter_H
