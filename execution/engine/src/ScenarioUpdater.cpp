#include "ScenarioUpdater.h"
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/events/ClockTickEvent.h"
#include "execution/engine/src/SystemState.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/data/src/DynamicScenario.h"
#include "model/data/src/StochasticScenario.h"

using namespace BPMNOS::Execution;

void ScenarioUpdater::subscribe(Engine* engine) {
  engine->addSubscriber(this, Observable::Type::Event);
  engine->addSubscriber(this, Observable::Type::Token);
}

void ScenarioUpdater::notice(const Observable* observable) {
  // Handle ClockTick events - reveal deferred data
  if (observable->getObservableType() == Observable::Type::Event) {
    auto event = static_cast<const Event*>(observable);
    if (auto clockTickEvent = event->is<ClockTickEvent>()) {
      auto systemState = clockTickEvent->systemState;
      auto currentTime = systemState->getTime();

      if (auto dynamicScenario = dynamic_cast<const BPMNOS::Model::DynamicScenario*>(systemState->scenario)) {
        dynamicScenario->revealData(currentTime);
      }
      else if (auto stochasticScenario = dynamic_cast<const BPMNOS::Model::StochasticScenario*>(systemState->scenario)) {
        stochasticScenario->revealData(currentTime);
      }
    }
    return;
  }

  // Handle Token BUSY - set task completion status
  if (observable->getObservableType() == Observable::Type::Token) {
    auto token = static_cast<const Token*>(observable);

    if (!token->node ||
        !token->node->represents<BPMN::Task>() ||
        token->state != Token::State::BUSY) {
      return;
    }

    // Exclude SendTask, ReceiveTask, DecisionTask
    if (token->node->represents<BPMN::SendTask>() ||
        token->node->represents<BPMN::ReceiveTask>() ||
        token->node->represents<BPMNOS::Model::DecisionTask>()) {
      return;
    }

    auto instanceId = token->getInstanceId();
    token->owner->systemState->scenario->setTaskCompletionStatus(instanceId, token->node, token->status);
  }
}
