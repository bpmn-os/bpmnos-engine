#include "Engine.h"
#include "Token.h"
#include "StateMachine.h"
#include "SequentialPerformerUpdate.h"
#include "ConditionalEventObserver.h"
#include "execution/controller/src/Decision.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/engine/src/events/TimerEvent.h"
#include "execution/utility/src/erase.h"
#include <cassert>
#include <limits>
#include <stdexcept>

using namespace BPMNOS::Execution;

Engine::Engine()
{
  lastInstantiationTime = std::numeric_limits<BPMNOS::number>::lowest();
  addSubscriber(&conditionalEventObserver, Observable::Type::DataUpdate);
  addSubscriber(&scenarioUpdater, Observable::Type::Event, Observable::Type::Token);
  readyHandler.connect(this);
  taskCompletionHandler.connect(this);
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

void Engine::processCommands() {
  while ( commands.size() ) {
//std::cerr << "execute" << std::endl;
    // pop before executing, so that a command enqueueing further commands does not extend the list
    // behind the one being executed
    auto command = std::move(commands.front());
    commands.pop_front();
    command.execute();
  }
}


void Engine::initialize(const BPMNOS::Model::Scenario* scenario, BPMNOS::number startTime) {
  if ( startTime > scenario->getEarliestInstantiationTime() ) {
    throw std::logic_error("Engine: start time is later than the earliest instantiation time");
  }

  // create initial system state before the first instant of the run, so that the opening clock tick
  // advances time to it and time is reached the same way at the first instant as at every later one
  systemState = std::make_unique<SystemState>(this, scenario, std::numeric_limits<BPMNOS::number>::lowest());
  lastInstantiationTime = std::numeric_limits<BPMNOS::number>::lowest();
  commands.clear();
  conditionalEventObserver.connect( systemState.get() );
  // announce the installed state so subscribers (cached candidate sources) reset for the new run
  notify( systemState.get() );

  // open the run by advancing time to its first instant; the tick is announced like any other, so the
  // log states the time the run began at, and deferred data is disclosed for that instant
  ClockTickEvent clockTickEvent(systemState.get(), startTime);
  notify(&clockTickEvent);
  clockTickEvent.processBy(this);
}

void Engine::run(const BPMNOS::Model::Scenario* scenario, BPMNOS::number startTime, BPMNOS::number endTime) {
  initialize(scenario, startTime);
  run(endTime);
}

void Engine::run(BPMNOS::number endTime) {
  // advance all tokens in system state (state setup is done where the state is installed)
  while ( advance(endTime) ) {}
}

void Engine::initializeSystemState(const BPMNOS::Model::Scenario* scenario, const SystemState* foreignState) {
  // install a deep copy of the foreign state as this engine's own state; the copy already holds every
  // instance due up to its current time, so the instantiation watermark starts at that time
  systemState = std::make_unique<SystemState>(this, scenario, foreignState);
  lastInstantiationTime = systemState->getTime();
  // installing a new state resets the run state and binds the conditional-event observer to it
  commands.clear();
  conditionalEventObserver.connect( systemState.get() );
  // announce the initialized state so subscribers can build their data structures
  notify( systemState.get() );
}

void Engine::resume(BPMNOS::number endTime) {
  if ( !systemState ) {
    throw std::logic_error("Engine: resume requires an existing system state (call run or initializeSystemState first)");
  }
  // continue advancing the existing system state (no new state is created)
  run(endTime);
}

void Engine::resume(std::shared_ptr<Decision> decision, BPMNOS::number endTime) {
  resume(std::shared_ptr<Event>(decision), endTime);
}


void Engine::resume(std::shared_ptr<Event> event, BPMNOS::number endTime) {
  if ( !systemState ) {
    throw std::logic_error("Engine: resume requires an existing system state (call run or initializeSystemState first)");
  }
  if ( event->expired() ) {
    // a controller must check Event::expired() before forcing an event; guard against a stale one
    throw std::logic_error("Engine: event to resume is expired");
  }
  if ( event->is<TerminationEvent>() ) {
    // resuming only to terminate is forbidden
    throw std::logic_error("Engine: TerminationEvent is not allowed for resume");
  }
  // the run continues with the given decision
  notify(event.get());
  event->processBy(this);
  run(endTime);
}

bool Engine::advance(BPMNOS::number endTime) {
  if ( !systemState ) {
    throw std::logic_error("Engine: advance requires an existing system state (call initialize, run or initializeSystemState first)");
  }
  // every enqueued command is executed by whoever enqueued it, so none is outstanding here
  assert(commands.empty());

  // fetch and process a single event
  auto event = fetchEvent(systemState.get());
  if ( !event ) {
    // No event available, terminate
    return false;
  }

  if ( event->expired() ) {
    // the engine assumes only non-expired events are dispatched; a controller that is not safe by
    // design must check Event::expired() before dispatching. Guard against a stale event here.
    throw std::logic_error("Engine: event fetched is expired");
  }
  // stop before processing clock tick that would exceed endTime
  if ( auto clockTickEvent = event->is<ClockTickEvent>(); clockTickEvent && clockTickEvent->time > endTime ) {
    return false;
  }
  notify(event.get());
  event->processBy(this);

  if ( event->is<ClockTickEvent>() ) {
//std::cerr << "ClockTick" << std::endl;
    // continue only while the state has something left to advance
    return systemState->isAlive();
  }

  if ( event->is<TerminationEvent>() ) {
    // the run was told to stop
    return false;
  }

  return true;
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
  lastInstantiationTime = systemState->getTime();
}

void Engine::deleteInstance(StateMachine* instance) {
//std::cerr << "deleteInstance" << std::endl;
  erase_ptr<StateMachine>(systemState->instances,instance);
}

void Engine::process(const ReadyEvent* event) {
  auto token_ptr = event->token.lock();
  assert( token_ptr );
  Token* token = const_cast<Token*>(token_ptr.get());
  systemState->tokensAwaitingReadyEvent.remove(token);

  auto& status = const_cast<ReadyEvent*>(event)->statusAttributes;
  token->status = std::move(status);
  token->status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = systemState->currentTime;

  if ( auto scope = token->node->represents<BPMN::Scope>() ) {
    auto& data = const_cast<ReadyEvent*>(event)->dataAttributes;
    const_cast<StateMachine*>(token->owner)->createChild(token, scope, std::move(data));
  }
  commands.emplace_back(std::bind(&Token::advanceToReady,token), token);
  
  token_ptr.reset();
  processCommands();
}

void Engine::process(const EntryEvent* event) {
//std::cerr << systemState->pendingEntryEvents.empty() << "EntryEvent " << event->token->jsonify().dump() << std::endl;
  auto token_ptr = event->token.lock();
  assert( token_ptr );
  Token* token = const_cast<Token*>(token_ptr.get());
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

  token_ptr.reset();
  processCommands();
}

void Engine::process(const ChoiceEvent* event) {
//std::cerr << "ChoiceEvent " << event.token->node->id << std::endl;
  auto token_ptr = event->token.lock();
  assert( token_ptr );
  Token* token = const_cast<Token*>(token_ptr.get());
  token->decisionRequest.reset();
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

  token_ptr.reset();
  processCommands();
}

void Engine::process(const CompletionEvent* event) {
//std::cerr << "CompletionEvent " << event.token->node->id << std::endl;
  auto token_ptr = event->token.lock();
  assert( token_ptr );
  Token* token = const_cast<Token*>(token_ptr.get());
  systemState->tokensAwaitingCompletionEvent.remove(token);
  // update token status
  token->status = std::move(event->status);

  commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);

  token_ptr.reset();
  processCommands();
}

void Engine::process(const MessageDeliveryEvent* event) {
  auto token_ptr = event->token.lock();
  assert( token_ptr );
  Token* token = const_cast<Token*>(token_ptr.get());
  token->decisionRequest.reset();
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

  message_ptr.reset();
  token_ptr.reset();
  processCommands();
}

void Engine::process(const ExitEvent* event) {
//std::cerr << "ExitEvent: " << event->token->jsonify().dump() << std::endl;
  auto token_ptr = event->token.lock();
  assert( token_ptr );
  Token* token = const_cast<Token*>(token_ptr.get());
  token->decisionRequest.reset();

  if ( token->node->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>() ) {
    token->releaseSequentialPerformer();
  }

  // update token status
  if ( event->exitStatus.has_value() ) {
    token->status = event->exitStatus.value();
  }

  commands.emplace_back(std::bind(&Token::advanceToExiting,token), token);

  token_ptr.reset();
  processCommands();
}

void Engine::process(const ErrorEvent* event) {
  auto token_ptr = event->token.lock();
  assert( token_ptr );
  Token* token = const_cast<Token*>(token_ptr.get());
  commands.emplace_back(std::bind(&Token::advanceToFailed,token), token);

  token_ptr.reset();
  processCommands();
}

void Engine::process([[maybe_unused]] const ClockTickEvent* event) {
//std::cerr << "ClockTickEvent " << std::endl;
  systemState->increaseTimeTo(event->time);

  // add new instances that have become due at the new time; instantiation is caused by time advancing,
  // and time advances here and nowhere else
  if (lastInstantiationTime < systemState->getTime()) {
    addInstances();
  }

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

  processCommands();
}

void Engine::process([[maybe_unused]] const TerminationEvent* event) {
}


BPMNOS::number Engine::getCurrentTime() const {
  return systemState->currentTime;
}

const SystemState* Engine::getSystemState() const {
  return systemState.get();
}

