#include "TaskCompletionHandler.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/events/CompletionEvent.h"

using namespace BPMNOS::Execution;

std::shared_ptr<Event> TaskCompletionHandler::dispatchEvent(const SystemState* systemState) {
  auto currentTime = systemState->getTime();
  for (auto [time, token_ptr] : systemState->tokensAwaitingCompletionEvent) {
    if (time <= currentTime) {
      if (auto token = token_ptr.lock()) {
        auto instanceId = token->getInstanceId();
        auto status = systemState->scenario->getTaskCompletionStatus(instanceId, token->node, currentTime);
        if (status) {
          return std::make_shared<CompletionEvent>(token.get(), std::move(*status));
        }
      }
    }
  }
  return nullptr;
}
