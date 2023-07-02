#ifndef BPMNOS_Status_H
#define BPMNOS_Status_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "xml/bpmnos/tStatus.h"
#include "xml/bpmnos/tRestrictions.h"
#include "xml/bpmnos/tOperators.h"

namespace BPMNOS {

class Status : public BPMN::ExtensionElements {
public:
  Status(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent=nullptr);
  std::vector< std::reference_wrapper<XML::bpmnos::tAttribute> > status;
  std::vector< std::reference_wrapper<XML::bpmnos::tRestriction> > restrictions;
  std::vector< std::reference_wrapper<XML::bpmnos::tOperator> > operators;
};

} // namespace BPMNOS

#endif // BPMNOS_Status_H
