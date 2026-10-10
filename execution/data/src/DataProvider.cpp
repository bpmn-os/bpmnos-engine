#include "DataProvider.h"
#include "Scenario.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/events/ClockTickEvent.h"

using namespace BPMNOS::Execution;

DataProvider::DataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, std::chrono::milliseconds clockTickDuration)
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

const std::shared_ptr<const BPMNOS::Object>& DataProvider::getDefaultObject(const BPMNOS::Model::Attribute* object) const {
  auto it = defaultObjects.find(object);
  if ( it == defaultObjects.end() || !it->second ) {
    throw std::runtime_error("DataProvider: object '" + object->id + "' has an open dimension and no value");
  }
  return it->second;
}

void DataProvider::advance(const SystemState* systemState, Scenario& scenario, EventQueue& queue) const {
  auto now = std::chrono::steady_clock::now();
  // the elapsed time is compared in milliseconds rather than the duration added to the previous clock tick,
  // so that the maximum duration, with which the data provider never advances time itself, cannot overflow
  if ( std::chrono::duration_cast<std::chrono::milliseconds>(now - scenario.previousClockTick) < clockTickDuration ) {
    // the clock tick is not yet due; the controller may still decide meanwhile
    return;
  }
  scenario.previousClockTick = now;
  queue.push_back(std::make_shared<ClockTickEvent>(systemState));
}
