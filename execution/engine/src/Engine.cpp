#include "Engine.h"
#include "model/utility/src/Keywords.h"

using namespace BPMNOS::Execution;

Engine::Engine(BPMNOS::Model::DataProvider* dataProvider) : dataProvider(dataProvider) {
  timestamp = 0;

  // get instances
  auto& instances = dataProvider->getInstances();
  for ( auto& [id,instance] : instances ) {
    auto timestampAttribute = instance->getAttributeById(BPMNOS::Keyword::Timestamp);
    std::optional<BPMNOS::number> startTime;
    if ( startTime = instance->getActualValue(timestampAttribute); startTime.has_value() ) {
      // start time is known with certainty
    }
    else if ( startTime = instance->getAssumedValue(timestampAttribute); startTime.has_value() ) {
      // start time is assumed to take a certain value
    }
    else if ( startTime = instance->getPredictedValue(timestampAttribute); startTime.has_value() ) {
      // actual start time may differ from prediction
    }
    else {
      // actual start time is completely unknown
      startTime = std::numeric_limits<BPMNOS::number>::max();
    }

    AnticipatedInstantiation anticipation = { .anticipatedTime = startTime.value(), .instance = instance.get() };
    anticipatedInstances.insert( anticipation );
  }
}

void Engine::startInstances() {
  for (auto it = anticipatedInstances.begin(); it != anticipatedInstances.end(); ) {
    auto timestampAttribute = it->instance->getAttributeById(BPMNOS::Keyword::Timestamp);

    std::optional<BPMNOS::number> startTime;
    if ( startTime = it->instance->getActualValue(timestampAttribute); startTime.has_value() ) {
      // start time is known with certainty
    }
    else if ( startTime = it->instance->getAssumedValue(timestampAttribute); startTime.has_value() ) {
      // start time is assumed to take a certain value
    }

    if ( startTime.has_value() && startTime.value() <= getTimestamp() ) {
      startInstance( it->instance );
      it = anticipatedInstances.erase(it);
    }
    else {
        ++it;
    }
  }
}

void Engine::startInstance(const BPMNOS::Model::InstanceData* instance) {
  runningInstances.push_back(std::make_unique<StateMachine>(instance));
}

BPMNOS::number Engine::getTimestamp() {
  return timestamp;
}

