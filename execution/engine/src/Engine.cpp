#include "Engine.h"

using namespace BPMNOS;

Engine::Engine(DataProvider* dataProvider) : dataProvider(dataProvider) {
  timestamp = 0;
}

void Engine::start() {
  // create tokens
  auto& instances = dataProvider->getInstances();
  for ( auto& [id,instance] : instances ) {
  }
}

number Engine::getTimestamp() {
  return timestamp;
}

