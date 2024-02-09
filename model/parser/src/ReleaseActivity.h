#ifndef BPMNOS_Model_ReleaseActivity_H
#define BPMNOS_Model_ReleaseActivity_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>
#include "xml/bpmnos/tAllocations.h"
#include "xml/bpmnos/tAllocation.h"

namespace BPMNOS::Model {

class ReleaseActivity : public BPMN::SubProcess {
  friend class Model;
public:
  ReleaseActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent);
};

} // namespace BPMNOS::Model

#endif // BPMNOS_Model_ReleaseActivity_H
