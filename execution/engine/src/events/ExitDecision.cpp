#include "ExitDecision.h"
#include "execution/engine/src/Engine.h"

using namespace BPMNOS::Execution;

ExitDecision::ExitDecision(const Token* token, std::optional<Values> exitStatus)
  : Decision(token)
  , exitStatus(exitStatus)
{
}

void ExitDecision::processBy(Engine* engine) const {
  engine->process(this);
}
