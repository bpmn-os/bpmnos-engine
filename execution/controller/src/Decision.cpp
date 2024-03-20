#include "Decision.h"

using namespace BPMNOS::Execution;


Decision::Decision(std::function<std::optional<double>(Event* event)> evaluator)
  : Event(nullptr), evaluator(evaluator)
{
}

