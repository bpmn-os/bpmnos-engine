#ifndef BPMNOS_ResourceActivity_H
#define BPMNOS_ResourceActivity_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>

namespace BPMNOS::Model {

class ResourceActivity : public BPMN::SubProcess {
  friend class Model;
public:
  ResourceActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent);
  std::vector<BPMN::Activity*> jobs;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_ResourceActivity_H
