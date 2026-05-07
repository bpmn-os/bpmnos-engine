#ifndef BPMNOS_Execution_Evaluation_H
#define BPMNOS_Execution_Evaluation_H

#include <memory>
#include <optional>

namespace BPMNOS::Execution {

struct Evaluation : std::enable_shared_from_this<Evaluation> {
  std::optional<double> value;
  Evaluation(std::optional<double> v) : value(v) {}
};

} // namespace BPMNOS::Execution

#endif // BPMNOS_Execution_Evaluation_H
