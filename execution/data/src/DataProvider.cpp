#include "DataProvider.h"
#include "Scenario.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/events/ClockTickEvent.h"

using namespace BPMNOS::Execution;

DataProvider::DataProvider(const BPMNOS::Model::Model* model, unsigned int clockTickDuration)
  : clockTickDuration(clockTickDuration)
  , model(model)
{
}

const BPMNOS::Model::Model* DataProvider::getModel() const {
  return model;
}

void DataProvider::advance(const SystemState* systemState, Scenario& scenario, EventQueue& queue) const {
  auto now = std::chrono::steady_clock::now();
  if ( now < scenario.previousClockTick + std::chrono::milliseconds(clockTickDuration) ) {
    // the clock tick is not yet due; the controller may still decide meanwhile
    return;
  }
  scenario.previousClockTick = now;
  queue.push_back(std::make_shared<ClockTickEvent>(systemState));
}
