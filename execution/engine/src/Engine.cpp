#include "Engine.h"
#include "Token.h"
#include "StateMachine.h"
#include "SequentialPerformerUpdate.h"
#include "ConditionalEventObserver.h"
#include "execution/controller/src/Decision.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/SignalDefinition.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "execution/engine/src/events/TimerEvent.h"
#include "execution/utility/src/erase.h"
#include "execution/data/src/LegacyDataProvider.h" // TRANSITIONAL: see the entry points taking a Model::Scenario
#include <cassert>
#include <limits>
#include <stdexcept>
#include <thread>
#include <chrono>

using namespace BPMNOS::Execution;

Engine::Engine(std::shared_ptr<const BPMNOS::Model::Model> model)
  : Engine()
{
  sharedModel = std::move(model);
  this->model = sharedModel.get();
}

Engine::Engine()
{
  addSubscriber(&conditionalEventObserver, Observable::Type::DataUpdate);
  environment.connect(this);
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


void Engine::acceptScenario(const BPMNOS::Model::Scenario* scenario) {
  if ( !model ) {
    model = scenario->getModel();
  }
  else if ( scenario->getModel() != model ) {
    throw std::invalid_argument("Engine: the scenario is not one of the model of the engine");
  }
}

const BPMNOS::Model::Model* Engine::getModel() const {
  return model;
}

void Engine::initialize(const BPMNOS::Model::Scenario* scenario, BPMNOS::number startTime) {
  // TRANSITIONAL: the scenario is wrapped here until callers pass a scenario of a data provider themselves;
  // removed with the entry points taking a Model::Scenario
  initialize(LegacyDataProvider::wrap(scenario), startTime);
}

void Engine::initialize(std::unique_ptr<LegacyDataProvider::Scenario> legacyScenario, BPMNOS::number startTime) {
  auto scenario = legacyScenario->scenario;
  acceptScenario(scenario);
  if ( startTime > scenario->getEarliestInstantiationTime() ) {
    throw std::logic_error("Engine: start time is later than the earliest instantiation time");
  }

  // create initial system state before the first instant of the run, so that the opening clock tick
  // advances time to it and time is reached the same way at the first instant as at every later one
  systemState = std::make_unique<SystemState>(this, scenario, std::numeric_limits<BPMNOS::number>::lowest());
  environment.setScenario(std::move(legacyScenario));
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

void Engine::run(std::unique_ptr<LegacyDataProvider::Scenario> scenario, BPMNOS::number startTime, BPMNOS::number endTime) {
  initialize(std::move(scenario), startTime);
  run(endTime);
}

void Engine::run(BPMNOS::number endTime) {
  // advance all tokens in system state (state setup is done where the state is installed)
  while ( advance(endTime) ) {}
}

void Engine::initializeSystemState(const BPMNOS::Model::Scenario* scenario, const SystemState* foreignState) {
  // TRANSITIONAL: the scenario is wrapped here until callers pass a scenario of a data provider themselves;
  // removed with the entry points taking a Model::Scenario
  initializeSystemState(LegacyDataProvider::wrap(scenario), foreignState);
}

void Engine::initializeSystemState(std::unique_ptr<LegacyDataProvider::Scenario> legacyScenario, const SystemState* foreignState) {
  auto scenario = legacyScenario->scenario;
  acceptScenario(scenario);
  // install a deep copy of the foreign state as this engine's own state; the copy already holds every
  // instance known up to its current time
  systemState = std::make_unique<SystemState>(this, scenario, foreignState);
  environment.setScenario(std::move(legacyScenario));
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

  // a round asks the environment for an event due at the current time, then the dispatchers of the
  // controller, and, if neither supplies one, asks the environment to advance
  auto event = environment.dispatchEvent(systemState.get());
  if ( !event ) {
    event = fetchEvent(systemState.get());
  }
  if ( !event ) {
    event = environment.advance(systemState.get());
  }
  if ( !event ) {
    // the run waits for the wall clock or for events from outside the run; pause for a moment, so that the
    // engine does not ask in vain at full speed
    std::this_thread::sleep_for(SLEEP);
    return true;
  }

  if ( event->expired() ) {
    // the engine assumes only non-expired events are dispatched; a controller that is not safe by
    // design must check Event::expired() before dispatching. Guard against a stale event here.
    throw std::logic_error("Engine: event fetched is expired");
  }
  // TRANSITIONAL: stop before processing a clock tick that would exceed endTime; removed once the end time is
  // set on the data provider
  if ( auto clockTickEvent = event->is<ClockTickEvent>(); clockTickEvent && clockTickEvent->time > endTime ) {
    return false;
  }
  notify(event.get());
  event->processBy(this);

  if ( event->is<TerminationEvent>() ) {
    // the run was told to stop
    return false;
  }

  return true;
}

void Engine::broadcastSignal(Signal signal) {
  // the signal is announced before it is delivered anywhere, so that what is observed is the signal
  // rather than what it causes
  notify(&signal);

  auto& waitingTokens = systemState->tokensAwaitingSignal[signal.name];
  for ( auto& [token_ptr] : waitingTokens ) {
    auto token = token_ptr.lock();
    assert( token );
    // receive signal content
    token->setSignalContent(signal.content);

    // advance receiving token
    commands.emplace_back(std::bind(&Token::advanceToCompleted,token.get()), token.get());
  }
  waitingTokens.clear();

  // instantiate the process the signal triggers, if any
  auto& processesTriggeredBySignal = model->processesTriggeredBySignal;
  if ( auto it = processesTriggeredBySignal.find(signal.name); it != processesTriggeredBySignal.end() ) {
    // the instantiation is enqueued rather than performed here, so that a process throwing the signal
    // instantiating it does not recurse through the stack of the broadcast
    commands.emplace_back( std::bind(&Engine::triggerInstance, this, it->second, signal.content) );
  }
}

void Engine::triggerInstance(const BPMN::Process* process, BPMNOS::VariedValueMap content) {
  auto extensionElements = process->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  auto& attributeRegistry = extensionElements->attributeRegistry;

  // the identifier is generated, an instance created by a trigger being declared nowhere
  auto counter = ++systemState->instantiationCounter[process];
  auto instanceId = process->id + BPMNOS::Model::Scenario::delimiters[1] + std::to_string(counter);

  BPMNOS::Values data( extensionElements->data.size() );
  data[Model::ExtensionElements::Index::Instance] = BPMNOS::to_number(instanceId,STRING);

  BPMNOS::Values status( extensionElements->attributes.size() );
  status[Model::ExtensionElements::Index::Timestamp] = systemState->getTime();

  // the content of the trigger is applied to the initial status of the instance, which is where it is
  // needed and after which it is of no further concern
  assert( process->startNodes.size() == 1 );
  auto startNode = process->startNodes.front();

  // a signal start event holds its definition as its extension elements, whereas a message start event
  // holds extension elements carrying a message definition
  const BPMNOS::Model::ContentMap* contentMap;
  const std::vector<const BPMNOS::Model::Attribute*>* dataUpdateAttributes;
  bool dataUpdateIsGlobal;
  if ( auto signalDefinition = startNode->extensionElements->represents<BPMNOS::Model::SignalDefinition>() ) {
    contentMap = &signalDefinition->contentMap;
    dataUpdateAttributes = &signalDefinition->dataUpdate.attributes;
    dataUpdateIsGlobal = signalDefinition->dataUpdate.global;
  }
  else {
    auto startNodeExtensionElements = startNode->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    contentMap = &startNodeExtensionElements->getMessageDefinition()->contentMap;
    dataUpdateAttributes = &startNodeExtensionElements->dataUpdate.attributes;
    dataUpdateIsGlobal = startNodeExtensionElements->dataUpdate.global;
  }

  auto oldObjective = systemState->globals[Model::ExtensionElements::Index::Objective];
  for ( auto& [key,definition] : *contentMap ) {
    auto attribute = definition->attribute;
    auto it = content.find(key);
    if ( it == content.end() ) {
      // key in content of start event, but not in content of the trigger
      attributeRegistry.setValue(attribute, status, data, systemState->globals, std::nullopt );
    }
    else if ( std::holds_alternative< std::optional<BPMNOS::number> >(it->second) ) {
      attributeRegistry.setValue(attribute, status, data, systemState->globals, std::get< std::optional<BPMNOS::number> >(it->second) );
    }
    else {
      // use default value of emitter
      ValueVariant value = std::get< std::string >(it->second);
      attributeRegistry.setValue(attribute, status, data, systemState->globals, BPMNOS::to_number(value,attribute->type) );
    }
  }

  if ( systemState->globals[Model::ExtensionElements::Index::Objective] != oldObjective ) {
    // dataUpdate indicating that objective has changed
    notify( DataUpdate( { attributeRegistry.globalAttributes[Model::ExtensionElements::Index::Objective] } ) );
  }

  if ( dataUpdateIsGlobal ) {
    // notify about data update; the instance does not exist yet, so only a global update can be reported
    notify( DataUpdate( *dataUpdateAttributes ) );
  }

  systemState->instances.push_back(std::make_shared<StateMachine>(systemState.get(),process,std::move(data),std::move(status)));

  // the trigger is the condition for the start, so the instance is started at once and not upon a ready
  // event: the child is created and the token at the process advances through READY
  auto instance = systemState->instances.back().get();
  auto token = instance->tokens.front().get();
  instance->createChild(token, process, {});
  token->advanceToReady();
}

void Engine::triggerInstanceByMessage(const BPMN::Process* process, std::weak_ptr<Message> message_ptr) {
  auto message = message_ptr.lock();
  if ( !message ) {
    // the message was withdrawn before it could be consumed
    return;
  }

  triggerInstance( process, message->contentValueMap );

  // the message is consumed here, no delivery event being dispatched for a message instantiating a process
  message->state = Message::State::DELIVERED;
  notify(message.get());
  erase_ptr<Message>(systemState->messages,message.get());

  if ( message->waitingToken ) {
    // send task is completed
    systemState->messageAwaitingDelivery.erase( message->waitingToken );
    commands.emplace_back(std::bind(&Token::advanceToCompleted,message->waitingToken), message->waitingToken);
  }
}

void Engine::process(const InstantiationEvent* event) {
  auto process = event->process;
  auto& status = const_cast<InstantiationEvent*>(event)->status;
  auto& data = const_cast<InstantiationEvent*>(event)->data;
  if ( !data[Model::ExtensionElements::Index::Instance].has_value() ) {
    throw std::runtime_error("Engine: instance of process '" + process->id + "' has no id");
  }
  if ( !status[Model::ExtensionElements::Index::Timestamp].has_value() ) {
    throw std::runtime_error("Engine: instance of process '" + process->id + "' has no timestamp");
  }
  systemState->instantiationCounter[process]++;
  systemState->instances.push_back(std::make_shared<StateMachine>(systemState.get(),process,std::move(data),std::move(status)));
  // the token at the process awaits the ready event starting the instance
  systemState->instances.back()->tokens.front()->advanceFromCreated();

  processCommands();
}

void Engine::deleteInstance(StateMachine* instance) {
//std::cerr << "deleteInstance" << std::endl;
  erase_ptr<StateMachine>(systemState->instances,instance);
}

void Engine::process(const SignalBroadcastEvent* event) {
  // the signal is broadcast as a signal thrown within the model is
  commands.emplace_back( std::bind(&Engine::broadcastSignal, this, event->signal) );
  processCommands();
}

void Engine::process(const ReadyEvent* event) {
  auto token_ptr = event->token.lock();
  assert( token_ptr );
  Token* token = const_cast<Token*>(token_ptr.get());
  systemState->tokensAwaitingReadyEvent.remove(token);

  auto& status = const_cast<ReadyEvent*>(event)->statusAttributes;
  token->status = std::move(status);
  token->status[BPMNOS::Model::ExtensionElements::Index::Timestamp] = systemState->currentTime;

  if ( !token->node ) {
    // the token at a process starts the instance: the data owned by the state machine of the process is
    // replaced element by element, so that the references to it remain valid, and the child owning the
    // tokens flowing through the process is created
    auto stateMachine = const_cast<StateMachine*>(token->owner);
    auto& data = const_cast<ReadyEvent*>(event)->dataAttributes;
    assert( data.size() == stateMachine->data.size() );
    for ( size_t i = 0; i < data.size(); i++ ) {
      stateMachine->data[i].get() = data[i];
    }
    stateMachine->createChild(token, stateMachine->process, {});
  }
  else if ( auto scope = token->node->represents<BPMN::Scope>() ) {
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
  auto oldObjective = token->globals[BPMNOS::Model::ExtensionElements::Index::Objective];
  for (size_t i = 0; i < extensionElements->choices.size(); i++) {
    extensionElements->attributeRegistry.setValue( extensionElements->choices[i]->attribute, token->status, *token->data, token->globals, event->choices[i] );
  }
  if ( token->globals[BPMNOS::Model::ExtensionElements::Index::Objective] != oldObjective ) {
    // dataUpdate indicating that objective has changed
    notify( DataUpdate( { extensionElements->attributeRegistry.globalAttributes[BPMNOS::Model::ExtensionElements::Index::Objective] } ) );
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
  auto oldObjective = token->globals[BPMNOS::Model::ExtensionElements::Index::Objective];
  message->apply(token->node,token->getAttributeRegistry(),token->status,*token->data,token->globals);
  if ( token->globals[BPMNOS::Model::ExtensionElements::Index::Objective] != oldObjective ) {
    // dataUpdate indicating that objective has changed
    notify( DataUpdate( { token->getAttributeRegistry().globalAttributes[BPMNOS::Model::ExtensionElements::Index::Objective] } ) );
  }

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

