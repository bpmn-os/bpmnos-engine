#include "Engine.h"

using namespace BPMNOS::Execution;

Engine::Engine(BPMNOS::Model::DataProvider* dataProvider) : dataProvider(dataProvider) {
  timestamp = 0;
}

void Engine::start() {
  // create tokens
  auto& instances = dataProvider->getInstances();
  for ( auto& [id,instance] : instances ) {
  }
}

BPMNOS::number Engine::getTimestamp() {
  return timestamp;
}

