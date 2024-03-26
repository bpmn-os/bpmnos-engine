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
  Guidance(XML::bpmnos::tGuidance* guidance, AttributeMap statusAttributes);
  XML::bpmnos::tGuidance* element;
  AttributeMap statusAttributes; ///< Map allowing to look up attributes by their names.

  enum class Type { Entry, Exit, Choice, MessageDelivery };
  Type type;
  std::vector< std::unique_ptr<Attribute> > attributes;
  std::vector< std::unique_ptr<Restriction> > restrictions;
  std::vector< std::unique_ptr<Operator> > operators;

  std::optional<BPMNOS::number> apply(const Scenario* scenario, BPMNOS::number currentTime, const std::string& instanceId, const BPMN::FlowNode* node, Values& status) const;
private:  
  BPMNOS::number getObjective(const Values& values) const;
  bool restrictionsSatisfied(const Values& values) const;

};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Guidance_H
