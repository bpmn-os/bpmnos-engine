#ifndef BPMNOS_Decisions_H
#define BPMNOS_Decisions_H

#include <memory>
#include <unordered_map>
#include <string>
#include <bpmn++.h>
#include "Status.h"
#include "Decision.h"

namespace BPMNOS {

class Decisions : public Status {
public:
  Decisions(XML::bpmn::tBaseElement* baseElement, BPMN::Scope* parent);
  std::vector< std::unique_ptr<Decision> > decisions;

  void makeChoices(const std::unordered_map<Decision*,number>& choices, Values& values) const;
};

} // namespace BPMNOS

#endif // BPMNOS_Decisions_H
