#ifndef BPMNOS_Decisions_H
#define BPMNOS_Decisions_H

#include <memory>
#include <vector>
#include <string>
#include <bpmn++.h>
#include "Status.h"
#include "Decision.h"

namespace BPMNOS {

class Decisions : public Status {
public:
  Decisions(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  std::vector< std::unique_ptr<Decision> > decisions;
};

} // namespace BPMNOS

#endif // BPMNOS_Decisions_H
