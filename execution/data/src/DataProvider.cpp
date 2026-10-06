#include "DataProvider.h"
#include "Scenario.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/events/ClockTickEvent.h"

using namespace BPMNOS::Execution;

DataProvider::DataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, unsigned int clockTickDuration)
  : clockTickDuration(clockTickDuration)
  , model(std::move(model))
{
}

std::unique_ptr<Scenario> DataProvider::forkScenario([[maybe_unused]] const Scenario& scenario, [[maybe_unused]] unsigned int index) const {
  // the future of the run is certain, so that its fork is a new scenario
  return createScenario();
}

const std::shared_ptr<const BPMNOS::Model::Model>& DataProvider::getModel() const {
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
