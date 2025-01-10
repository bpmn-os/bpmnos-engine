#include "Engine.h"
#include "Token.h"
#include "StateMachine.h"
#include "SequentialPerformerUpdate.h"
#include "ConditionalEventObserver.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/engine/src/events/TimerEvent.h"
#include "execution/utility/src/erase.h"
#include <cassert>

using namespace BPMNOS::Execution;

Engine::Engine()
{
  clockTick = 1;
}

Engine::~Engine()
{
//std::cerr << "~Engine()" << std::endl;
}

void Engine::Command::execute() {
  if ( token_ptr.has_value() && token_ptr->expired() ) {
    // relevant token no longer exists, skip command
    return;
  }

  if ( stateMachine_ptr.has_value() && stateMachine_ptr->expired() ) {
    // relevant state machine no longer exists, skip command
    return;
  }

  function();
}

BPMNOS::number Engine::run(const BPMNOS::Model::Scenario* scenario, BPMNOS::number timeout) {
  // create initial system state
  systemState = std::make_unique<SystemState>(this, scenario);
  conditionalEventObserver = std::make_unique<ConditionalEventObserver>(systemState.get());
  addSubscriber(conditionalEventObserver.get(), Observable::Type::DataUpdate);

  // advance all tokens in system state
  while ( advance() ) {
//std::cerr << ".";
    if ( !systemState->isAlive() ) {
std::cerr << "dead" << std::endl;
      break;
    }
    if ( systemState->getTime() > timeout ) {
std::cerr << "timeout" << std::endl;
      break;
    }
  }
  
  // get final objective value
  return systemState->getObjective();
//  std::cout << "Objective (maximization): " << (float)objective << std::endl;
//  std::cout  << "Objective (minimization): " << -(float)objective << std::endl;
}

bool Engine::advance() {
  // make sure new data is added to system state
  const_cast<BPMNOS::Model::Scenario*>(systemState->scenario)->update();

  // add all new instances and advance tokens
  addInstances();
  // it is assumed that at least one clock tick or termination event is processed to ensure that
  // each instance is only added once

  while ( commands.size() ) {
//std::cerr << "execute" << std::endl;
    auto command = std::move(commands.front());
    commands.pop_front();
    command.execute();
  }

  // fetch and process all events
  while ( auto event = fetchEvent(systemState.get()) ) {
//std::cerr << "*";
    notify(event.get());
    event->processBy(this);

    while ( commands.size() ) {
      commands.front().execute();
      commands.pop_front();
    }

    if ( event->is<ClockTickEvent>() ) {
//std::cerr << "ClockTick" << std::endl;
      // exit loop to resume
      return true;
    }

    if ( event->is<TerminationEvent>() ) {
      // exit loop to terminate
      return false;
    }
  }

  throw std::runtime_error("Engine: unexpected absence of event");
}

void Engine::addInstances() {
  for (auto& [process,status,data] : systemState->getInstantiations() ) {
    if ( !process->isExecutable ) {
      throw std::runtime_error("Engine: process '" + process->id + "' is not executable");
    }
    if ( !data[Model::ExtensionElements::Index::Instance].has_value() ) {
      throw std::runtime_error("Engine: instance of process '" + process->id + "' has no id");
    }
    if ( !status[Model::ExtensionElements::Index::Timestamp].has_value() ) {
      throw std::runtime_error("Engine: instance of process '" + process->id + "' has no timestamp");
    }
    systemState->instantiationCounter++;
    systemState->instances.push_back(std::make_shared<StateMachine>(systemState.get(),process,std::move(data)));
    // run instance and advance token
    systemState->instances.back()->run(std::move(status));
  }
}

void Engine::deleteInstance(StateMachine* instance) {
//std::cerr << "deleteInstance" << std::endl;
  erase_ptr<StateMachine>(systemState->instances,instance);
}

void Engine::process(const ReadyEvent* event) {
//std::cerr << "ReadyEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event->token);
  token->status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = systemState->currentTime;
  systemState->tokensAwaitingReadyEvent.remove(token);

  token->sequenceFlow = nullptr;
  token->status.insert(token->status.end(), event->statusAttributes.begin(), event->statusAttributes.end());

  if ( auto scope = token->node->represents<BPMN::Scope>() ) {
    const_cast<StateMachine*>(token->owner)->createChild(token, scope, event->dataAttributes);
  }
  commands.emplace_back(std::bind(&Token::advanceToReady,token), token);
}

void Engine::process(const EntryEvent* event) {
//std::cerr << systemState->pendingEntryEvents.empty() << "EntryEvent " << event->token->jsonify().dump() << std::endl;
  Token* token = const_cast<Token*>(event->token);
  token->decisionRequest.reset();
  token->status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = systemState->currentTime;
  if ( token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    token->occupySequentialPerformer();
  }

  // update token status
  if ( event->entryStatus.has_value() ) {
    token->status = event->entryStatus.value();
  }

  commands.emplace_back(std::bind(&Token::advanceToEntered,token), token);

//  systemState->pendingEntryEvents.remove(token);
}

void Engine::process(const ChoiceEvent* event) {
//std::cerr << "ChoiceEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event->token);
  token->status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = systemState->currentTime;
  assert( token->node );
  assert( token->node->represents<BPMNOS::Model::DecisionTask>() );

  auto extensionElements = token->node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  assert( extensionElements );
  assert( extensionElements->choices.size() == event->choices.size() );
  // apply choices
  for (size_t i = 0; i < extensionElements->choices.size(); i++) {
    extensionElements->attributeRegistry.setValue( extensionElements->choices[i]->attribute, token->status, *token->data, token->globals, event->choices[i] );
  }

  commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);

  systemState->pendingChoiceEvents.remove(token);
}

void Engine::process(const CompletionEvent* event) {
//std::cerr << "CompletionEvent " << event.token->node->id << std::endl;
  Token* token = const_cast<Token*>(event->token);
  token->status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = systemState->currentTime;
  systemState->tokensAwaitingCompletionEvent.remove(token);
  // update token status
  if ( event->updatedStatus.has_value() ) {
    token->status = std::move( event->updatedStatus.value() );
  }

  commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);
}

void Engine::process(const MessageDeliveryEvent* event) {
  Token* token = const_cast<Token*>(event->token);
  token->status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = systemState->currentTime;
  assert( token->node );

  auto message_ptr = event->message.lock();
  assert( message_ptr );
  Message* message = const_cast<Message*>(message_ptr.get());
  // update token status 
  message->apply(token->node,token->getAttributeRegistry(),token->status,*token->data,token->globals);
  
  message->state = Message::State::DELIVERED;
  notify(message);
  
  erase_ptr<Message>(systemState->messages,message);

  if ( message->waitingToken ) {
    // send task is completed
    systemState->messageAwaitingDelivery.erase( message->waitingToken );
    commands.emplace_back(std::bind(&Token::advanceToCompleted,message->waitingToken), message->waitingToken);
  }
  
  commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);

  systemState->pendingMessageDeliveryEvents.remove(token);
}

void Engine::process(const ExitEvent* event) {
//std::cerr << "ExitEvent: " << event->token->jsonify().dump() << std::endl;
  Token* token = const_cast<Token*>(event->token);
  token->decisionRequest.reset();

  if ( token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    token->releaseSequentialPerformer();
  }

  // update token status
  if ( event->exitStatus.has_value() ) {
    token->status = event->exitStatus.value();
  }

  commands.emplace_back(std::bind(&Token::advanceToExiting,token), token);

  systemState->pendingExitEvents.remove(token);
}

void Engine::process(const ErrorEvent* event) {
  Token* token = const_cast<Token*>(event->token);
  commands.emplace_back(std::bind(&Token::advanceToFailed,token), token);
}

void Engine::process([[maybe_unused]] const ClockTickEvent* event) {
//std::cerr << "ClockTickEvent " << std::endl;
  systemState->incrementTimeBy(clockTick);
  // trigger tokens awaiting timer
  while ( !systemState->tokensAwaitingTimer.empty() ) {
    auto it = systemState->tokensAwaitingTimer.begin();
    auto [time, token_ptr] = *it;
    if ( time > systemState->getTime() ) {
      break;
    }
    auto token = token_ptr.lock();
    assert( token );
    notify(TimerEvent(token.get()));
    commands.emplace_back(std::bind(&Token::advanceToCompleted,token.get()), token.get());
    systemState->tokensAwaitingTimer.remove(token.get());
  }
}

void Engine::process([[maybe_unused]] const TerminationEvent* event) {
  throw std::runtime_error("Engine: TerminationEvent not yet implemented");
}


BPMNOS::number Engine::getCurrentTime() {
  return systemState->currentTime;
}

const SystemState* Engine::getSystemState() {
  return systemState.get();
}

