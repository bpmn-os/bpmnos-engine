#ifndef BPMNOS_Model_Gatekeeper_H
#define BPMNOS_Model_Gatekeeper_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Restriction.h"
#include "model/utility/src/Number.h"

namespace BPMNOS::Model {


/**
 * @brief Class holding extension elements representing gatekeeper conditions for sequence flows 
 **/
class Gatekeeper : public BPMN::ExtensionElements {
public:
  Gatekeeper(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  const BPMN::Scope* parent;
  std::vector< std::unique_ptr<Restriction> > conditions;

  template <typename DataType>
  bool conditionsSatisfied(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const;
};


} // namespace BPMNOS::Model

#endif // BPMNOS_Model_Gatekeeper_H
