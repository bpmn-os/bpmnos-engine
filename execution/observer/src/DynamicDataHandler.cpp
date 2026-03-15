#include "DynamicDataHandler.h"
#include "execution/engine/src/events/ClockTickEvent.h"
#include "execution/engine/src/SystemState.h"
#include "model/data/src/DynamicScenario.h"

using namespace BPMNOS::Execution;

void DynamicDataHandler::subscribe(Engine* engine) {
  engine->addSubscriber(this, Execution::Observable::Type::Event);
}

void DynamicDataHandler::notice(const Observable* observable) {
  if ( observable->getObservableType() != Execution::Observable::Type::Event ) {
    return;
  }

  auto event = static_cast<const Event*>(observable);
  if ( auto clockTickEvent = event->is<ClockTickEvent>() ) {
    auto systemState = clockTickEvent->systemState;
    if ( auto dynamicScenario = dynamic_cast<const BPMNOS::Model::DynamicScenario*>(systemState->scenario) ) {
      dynamicScenario->revealData(systemState->getTime());
    }
  }
}
