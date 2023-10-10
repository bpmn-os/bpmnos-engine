#include "TimerHandler.h"
#include "execution/engine/src/events/TriggerEvent.h"
#include "model/parser/src/extensionElements/Timer.h"

using namespace BPMNOS::Execution;

TimerHandler::TimerHandler()
{
}

std::unique_ptr<Event> TimerHandler::fetchEvent( const SystemState& systemState ) {
  for ( auto token : systemState.awaitingTimer ) {
    if ( auto timer = token->node->extensionElements->as<BPMNOS::Model::Timer>() ) {
      BPMNOS::number trigger = timer->earliest(token->status);
      if ( trigger <= systemState.getTime() ) {  
        return std::make_unique<TriggerEvent>(token);
      }
    }
  }
  return nullptr;
}

