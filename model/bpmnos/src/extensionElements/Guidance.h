#ifndef BPMNOS_Model_Guidance_H
#define BPMNOS_Model_Guidance_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Attribute.h"
#include "Restriction.h"
#include "Operator.h"
#include "model/bpmnos/src/xml/bpmnos/tGuidance.h"

namespace BPMNOS::Model {

class Scenario;

class Guidance {
public:
  Guidance(XML::bpmnos::tGuidance* guidance, const AttributeRegistry& attributeRegistry);
  XML::bpmnos::tGuidance* element;
  AttributeRegistry attributeRegistry; ///< Registry allowing to look up attributes by their names.

  enum class Type { Entry, Exit, Choice, MessageDelivery };
  Type type;
  std::vector< std::unique_ptr<Attribute> > attributes;
  std::vector< std::unique_ptr<Restriction> > restrictions;
  std::vector< std::unique_ptr<Operator> > operators;

  template <typename DataType>
  std::optional<BPMNOS::number> apply(const Scenario* scenario, BPMNOS::number currentTime, const BPMNOS::number instanceId, const BPMN::FlowNode* node, BPMNOS::Values& status, DataType& data) const;
private:  
  template <typename DataType>
  BPMNOS::number getObjective(const BPMNOS::Values& status, const DataType& data) const;

  template <typename DataType>
  bool restrictionsSatisfied(const BPMNOS::Values& status, const DataType& data) const;

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Guidance_H
