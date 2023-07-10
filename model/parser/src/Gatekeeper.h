#ifndef BPMNOS_Gatekeeper_H
#define BPMNOS_Gatekeeper_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "xml/bpmnos/tRestrictions.h"

namespace BPMNOS {


class Gatekeeper : public BPMN::ExtensionElements {
public:
  Gatekeeper(XML::bpmn::tBaseElement* baseElement);
  std::vector< std::reference_wrapper<XML::bpmnos::tRestriction> > restrictions;
};

} // namespace BPMNOS

#endif // BPMNOS_Gatekeeper_H
