#include "StochasticTaskCompletion.h"
#include "execution/engine/src/Engine.h"
#include "execution/engine/src/events/CompletionEvent.h"
#include "model/bpmnos/src/DecisionTask.h"
#include <cassert>

using namespace BPMNOS::Execution;

StochasticTaskCompletion::StochasticTaskCompletion() {
}

void StochasticTaskCompletion::subscribe(Engine* engine) {
  engine->addSubscriber(this, Observable::Type::Token);
}

void StochasticTaskCompletion::notice(const Observable* observable) {
  if (observable->getObservableType() != Observable::Type::Token) {
    return;
  }

  auto token = static_cast<const Token*>(observable);

  if (
    !token->node ||
    !token->node->represents<BPMN::Task>() ||
    token->state != Token::State::BUSY
  ) {
    return;
  }

  // Only regular tasks (SendTask and ReceiveTask wait for message delivery, DecisionTask waits for choice)
  if (
    token->node->represents<BPMN::SendTask>() ||
    token->node->represents<BPMN::ReceiveTask>() ||
    token->node->represents<BPMNOS::Model::DecisionTask>()
  ) {
    return;
  }

  Values statusUpdate = createStatusUpdate(token);
  auto completionTime = statusUpdate[BPMNOS::Model::ExtensionElements::Index::Timestamp].value();
  pendingCompletions.emplace(
    completionTime,
    const_cast<Token*>(token)->weak_from_this(), 
    std::move(statusUpdate)
  );
}

BPMNOS::Values StochasticTaskCompletion::createStatusUpdate(const Token* token) {
  // For now, just copy the current status (playback)
  // Later: apply stochastic modifications here
  return token->status;
}

std::shared_ptr<Event> StochasticTaskCompletion::dispatchEvent(const SystemState* systemState) {
  if ( pendingCompletions.empty() ) {
    return nullptr;
  }
  
  auto it = pendingCompletions.begin();
  auto [time, token_ptr, statusUpdate] = *it;                        
                                                                                           
  if (time > systemState->getTime()) {
    // Ordered by time, so all tasks complete in the future
    return nullptr;          
  }                                                 

  auto token = token_ptr.lock();
  assert(token);
  pendingCompletions.erase(it);                                                      
  return std::make_shared<CompletionEvent>( token.get(), std::move(statusUpdate) );    
}    

