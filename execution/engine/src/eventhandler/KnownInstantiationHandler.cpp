#include "KnownInstantiationHandler.h"
#include "execution/engine/src/events/ReadyEvent.h"
#include "model/parser/src/extensionElements/Status.h"

using namespace BPMNOS::Execution;

KnownInstantiationHandler::KnownInstantiationHandler()
{
}

std::unique_ptr<Event> KnownInstantiationHandler::fetchEvent( const SystemState& systemState ) {
  auto instantiations = systemState.scenario->getKnownInstantiations(systemState.currentTime);
  for ( auto& instantiation : instantiations ) {
    // check if system state already contains instance
    BPMNOS::number instanceId = stringRegistry(instantiation->identifier);
    auto isSame = [instanceId](const std::unique_ptr<StateMachine>& instance) {
        return instance->tokens.front()->status[BPMNOS::Model::Status::Index::Instance] == instanceId;
    };
    auto it = std::find_if(systemState.instances.begin(), systemState.instances.end(), isSame);
    if (it == systemState.instances.end()) {
      Values status = systemState.scenario->getKnownStatus(instantiation,systemState.currentTime);
      return std::make_unique<InstantiationEvent>(instantiation->process,status);
    }
  }
  return nullptr;
}

