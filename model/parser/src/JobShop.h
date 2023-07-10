#ifndef BPMNOS_Job_H
#define BPMNOS_Job_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>

namespace BPMNOS {

class ResourceActivity;

class JobShop : public BPMN::SubProcess {
  friend class Model;
public:
  JobShop(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent);
  ResourceActivity* resourceActivity;
protected:
  ResourceActivity* getResource();
};

} // namespace BPMNOS

#endif // BPMNOS_Job_H
