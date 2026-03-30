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

  // Handle Token state changes
  if (observable->getObservableType() == Observable::Type::Token) {
    auto token = static_cast<const Token*>(observable);
    auto scenario = token->owner->systemState->scenario;

    if (!token->node) {
      return;
    }

    // Handle ARRIVED/CREATED at Activity - notify scenario of arrival
    if (
      token->node->represents<BPMN::Activity>() &&
      (token->state == Token::State::ARRIVED || token->state == Token::State::CREATED)
    ) {
      auto instanceId = token->owner->root->instance.value();
      assert(token->data);
      scenario->noticeActivityArrival(instanceId, token->node, token->status, *token->data, token->globals);
    }

    // Handle BUSY at Task - notify scenario of running task
    if (
      token->node->represents<BPMN::Task>() &&
      token->state == Token::State::BUSY
    ) {
      // Exclude SendTask, ReceiveTask, DecisionTask
      if (
        token->node->represents<BPMN::SendTask>() ||
        token->node->represents<BPMN::ReceiveTask>() ||
        token->node->represents<BPMNOS::Model::DecisionTask>()
      ) {
        // completion status is determined by Engine
        return;
      }

      auto instanceId = token->owner->root->instance.value();
      assert(token->data);
      scenario->noticeRunningTask(instanceId, token->node, token->status, *token->data, token->globals);
    }
  }
}
