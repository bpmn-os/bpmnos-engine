#ifndef BPMNOS_Model_Conditions_H
#define BPMNOS_Model_Conditions_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Restriction.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {


/**
 * @brief Class holding extension elements representing conditions for sequence flows and conditional events
 **/
class Conditions : public BPMN::ExtensionElements {
public:
  Conditions(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  std::vector< std::unique_ptr<Restriction> > conditions;
  std::set<const Attribute*> dataDependencies; ///< Set containing all attributes used in any of the conditions.

  template <typename DataType>
  bool conditionsSatisfied(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;
};


} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Conditions_H
