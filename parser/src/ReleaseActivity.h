#ifndef BPMNOS_ReleaseActivity_H
#define BPMNOS_ReleaseActivity_H

#include <memory>
#include <vector>
#include <optional>
#include <bpmn++.h>
#include "xml/bpmnos/tAllocations.h"
#include "xml/bpmnos/tRelease.h"

namespace BPMNOS {

class ReleaseActivity : public BPMN::SubProcess {
  friend class Model;
public:
  ReleaseActivity(XML::bpmn::tSubProcess* subProcess, BPMN::Scope* parent);
  std::vector< std::reference_wrapper<XML::bpmnos::tRelease> > releases;
};

} // namespace BPMNOS

#endif // BPMNOS_ReleaseActivity_H
