#ifndef BPMNOS_Model_Choice_H
#define BPMNOS_Model_Choice_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "model/bpmnos/src/xml/bpmnos/tDecision.h"
#include "Attribute.h"
#include "AttributeRegistry.h"
#include "Restriction.h"

namespace BPMNOS::Model {

/**
 * @brief Class representing a choice to be made within a @ref BPMNOS::Model::DecisionTask.
 *
 * The choice to be made may be restricted through @ref BPMNOS::Model::Restriction "restrictions". 
 */
class Choice {
public:
  Choice(XML::bpmnos::tDecision* decision, const AttributeRegistry& attributeRegistry, const std::vector< std::unique_ptr<Restriction> >& restrictions);
  XML::bpmnos::tDecision* element;
  const AttributeRegistry& attributeRegistry;
  const std::vector< std::unique_ptr<Restriction> >& restrictions;
  Attribute* attribute;

  std::pair<BPMNOS::number,BPMNOS::number> getBounds(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const; ///< Returns the minimal and maximal value the attribute may take.
  std::optional< std::vector<BPMNOS::number> > getEnumeration(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const; ///< Returns the allowed values the attribute may take.
private:
  BPMNOS::number lowerBound;
  BPMNOS::number upperBound;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Choice_H
