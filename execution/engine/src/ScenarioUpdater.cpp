#include "ScenarioUpdater.h"
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/events/ClockTickEvent.h"
#include "execution/engine/src/SystemState.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/data/src/Scenario.h"

using namespace BPMNOS::Execution;

void ScenarioUpdater::subscribe(Engine* engine) {
  engine->addSubscriber(this, Observable::Type::Event);
  engine->addSubscriber(this, Observable::Type::Token);
}

void ScenarioUpdater::notice(const Observable* observable) {
  // Handle ClockTick events - announce the time the clock is advancing to
  if (observable->getObservableType() == Observable::Type::Event) {
    auto event = static_cast<const Event*>(observable);
    if (auto clockTickEvent = event->is<ClockTickEvent>()) {
      clockTickEvent->systemState->scenario->noticeClockTick(clockTickEvent->time);
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

    // Handle ARRIVED/CREATED at Activity - notify scenario of arrival.
    // Multi-instance instance tokens also pass through CREATED, but the activity's arrival
    // status is computed once for the main token and inherited by the instances; they must not
    // re-trigger the scenario (which would write orphaned arrival statuses and, for stochastic
    // scenarios, consume the activity's random stream). Exclude them here, mirroring ReadyHandler.
    if (
      token->node->represents<BPMN::Activity>() &&
      (token->state == Token::State::ARRIVED || token->state == Token::State::CREATED) &&
      !token->owner->systemState->tokenAtMultiInstanceActivity.contains(const_cast<Token*>(token))
    ) {
      auto instanceId = token->owner->root->instance.value();
      assert(token->data);
      scenario->noticeReadyPending(instanceId, token->node, token->status, *token->data, token->globals);
    }

    // Handle READY at Activity - the arrival status has been used and is no longer owed. The guard
    // mirrors the one above, so that only an arrival that was announced is discarded.
    if (
      token->node->represents<BPMN::Activity>() &&
      token->state == Token::State::READY &&
      !token->owner->systemState->tokenAtMultiInstanceActivity.contains(const_cast<Token*>(token))
    ) {
      scenario->noticeReady(token->getInstanceId(), token->node);
    }

    // Handle BUSY at Task - notify scenario of running task
    if (
      token->node->represents<BPMN::Task>() &&
      token->state == Token::State::BUSY
    ) {
      // Exclude SendTask, ReceiveTask, DecisionTask
      if (
        token->node->represents<BPMN::SendTask>() ||
        token->node->represents<BPMNOS::Model::DecisionTask>() ||
        token->node->represents<BPMN::ReceiveTask>()
      ) {
        // completion status is determined by Engine
        return;
      }

      auto instanceId = token->owner->root->instance.value();
      assert(token->data);
      scenario->noticeCompletionPending(instanceId, token->node, token->status, *token->data, token->globals);
    }

    // Handle COMPLETED at Task - the completion status has been used and is no longer owed. The
    // announcement came from either of two places, so the guard is not the one used for BUSY above:
    // this updater announces every task that is not a send, receive or decision task, while
    // Token::advanceToCompleted announces every send task and every receive or decision task that
    // carries extension elements. A receive or decision task without them is announced by neither.
    if (
      token->node->represents<BPMN::Task>() &&
      token->state == Token::State::COMPLETED
    ) {
      bool announced =
        token->node->represents<BPMN::SendTask>() ||
        (
          ( token->node->represents<BPMN::ReceiveTask>() || token->node->represents<BPMNOS::Model::DecisionTask>() )
          ? (bool)token->node->extensionElements->represents<BPMNOS::Model::ExtensionElements>()
          : true
        );
      if ( announced ) {
        scenario->noticeCompletion(token->getInstanceId(), token->node);
      }
    }
  }
}
