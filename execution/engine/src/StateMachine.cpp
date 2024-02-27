#include "Engine.h"
#include "StateMachine.h"
#include "Token.h"
#include "SystemState.h"
#include "Event.h"
#include "events/EntryEvent.h"
#include "execution/utility/src/erase.h"
#include "model/parser/src/extensionElements/ExtensionElements.h"
#include "model/parser/src/extensionElements/Timer.h"
#include "execution/utility/src/VectorRegistry.h"
#include "bpmn++.h"
#include <cassert>
#include <ranges>

using namespace BPMNOS::Execution;

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Process* process)
  : systemState(systemState)
  , process(process)
  , scope(process)
  , parentToken(nullptr)
{
//std::cerr << "StateMachine(" << scope->id  << "/" << this << " @ " << parentToken << ")" << std::endl;
}

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken)
  : systemState(systemState)
  , process(parentToken->owner->process)
  , scope(scope)
  , parentToken(parentToken)
{
//std::cerr << "cStateMachine(" << scope->id << "/" << this << " @ " << parentToken << ")" << " owned by :" << parentToken->owner << std::endl;
}

StateMachine::StateMachine(const StateMachine* other)
  : systemState(other->systemState)
  , process(other->process)
  , scope(other->scope)
  , parentToken(other->parentToken)
{
//std::cerr << "oStateMachine(" << scope->id << "/" << this << " @ " << parentToken << ")"  << " owned by :" << parentToken->owner << std::endl;
}

StateMachine::~StateMachine() {
//std::cerr << "~StateMachine(" << scope->id << "/" << this << " @ " << parentToken << ")" << std::endl;
  const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation.erase(this);
  const_cast<SystemState*>(systemState)->tokenAwaitingCompensationEventSubProcess.erase(this);
  unregisterRecipient();
}

void StateMachine::initiateBoundaryEvents(Token* token) {
//std::cerr << "initiateBoundaryEvents: " << token->node->id << std::endl;
  auto activity = token->node->as<BPMN::Activity>();
  assert( activity);

  if ( activity->loopCharacteristics.has_value() &&
    activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::Standard
  ) {
    // determine main token waiting for all instances
    auto mainToken = const_cast<SystemState*>(systemState)->tokenAtMultiInstanceActivity.at(token);

    if ( const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[mainToken].empty() ) {
      // create boundary event tokens for the main token
      token = mainToken; 
    }
    else {
      // tokens at boundary events have already been created 
      return;
    }
  }

  for ( auto node : activity->boundaryEvents ) {
    if ( !node->represents<BPMN::CompensateBoundaryEvent>() ) {
      initiateBoundaryEvent(token,node);
    }
  }
}

void StateMachine::initiateBoundaryEvent(Token* token, const BPMN::FlowNode* node) {
//  throw std::runtime_error("StateMachine: boundary events not yet implemented!");
  tokens.push_back( std::make_shared<Token>(this,node,token->status) );
  auto createdToken = tokens.back().get();
  const_cast<SystemState*>(systemState)->tokenAssociatedToBoundaryEventToken[createdToken] = token;
  const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[token].push_back(createdToken);
  createdToken->advanceToEntered();
}

void StateMachine::initiateEventSubprocesses(Token* token) {
//std::cerr << "initiate " << scope->eventSubProcesses.size() << " eventSubprocesses for token at " << (token->node ? token->node->id : process->id ) << "/" << parentToken << "/" << token << " owned by " << token->owner << std::endl;
  for ( auto& eventSubProcess : scope->eventSubProcesses ) {
    pendingEventSubProcesses.push_back(std::make_shared<StateMachine>(systemState, eventSubProcess, parentToken));
    auto pendingEventSubProcess = pendingEventSubProcesses.back().get();
//std::cerr << "Pending event subprocess has parent: " << pendingEventSubProcess->parentToken->jsonify().dump() << std::endl;

    pendingEventSubProcess->run(token->status);
  }
}

void StateMachine::createMultiInstanceActivityTokens(Token* token) {
  auto extensionElements = token->node->extensionElements->represents<const BPMNOS::Model::ExtensionElements>();
  assert( extensionElements != nullptr );

  std::vector< std::map< const Model::Attribute*, std::optional<BPMNOS::number> > > valueMaps;

  if ( extensionElements->loopCardinality.has_value() ) {
    // use provided cardinality to determine number of tokens 
    if ( extensionElements->loopCardinality.value()->attribute.has_value() &&
      token->status[ extensionElements->loopCardinality.value()->attribute->get().index ].has_value()
    ) {
      valueMaps.resize( (size_t)token->status[ extensionElements->loopCardinality.value()->attribute->get().index ].value() );
    }
    else if ( extensionElements->loopCardinality.value()->value.has_value() ) {
      valueMaps.resize( (size_t)(int)extensionElements->loopCardinality.value()->value.value().get() );
    }
    else {
      throw std::runtime_error("StateMachine: cannot determine cardinality for multi-instance activity '" + token->node->id +"'" );
    }
  }
  
  if ( extensionElements->messageDefinitions.size() ) {
    if ( valueMaps.empty() ) {
      valueMaps.resize(extensionElements->messageDefinitions.size());
    }
    else if ( valueMaps.size() != extensionElements->messageDefinitions.size() ) {
      throw std::runtime_error("StateMachine: cardinality and number of messages inconsistent for multi-instance activity '" + token->node->id +"'" );
    }
  }

  auto attributes = extensionElements->attributes | std::views::filter([](auto& attribute) {
    return (attribute->collection != nullptr);
  });

  for ( auto& attribute : attributes ) {
    BPMNOS::number collectionIndex;
    
    if ( attribute->collection->attribute.has_value() &&
      token->status[ attribute->collection->attribute->get().index ].has_value()
    ) {
      collectionIndex = token->status[ attribute->collection->attribute->get().index ].value();
    }
    else if ( attribute->collection->value.has_value() ) {
      collectionIndex = BPMNOS::to_number( attribute->collection->value.value().get().value, STRING );
    }
    else {
      throw std::runtime_error("StateMachine: cannot determine values for multi-instance activity '" + token->node->id +"'");
    }

    auto& collection = vectorRegistry[(long unsigned int)collectionIndex];
    if ( valueMaps.empty() ) {
      valueMaps.resize(collection.size());
    }
    else if ( valueMaps.size() != collection.size() ) {
      throw std::runtime_error("StateMachine: inconsistent number of values provided for multi-instance activity '" + token->node->id +"'" );
    }

    // add value for each token copy
    for ( size_t i = 0; i < collection.size(); i++ ) {
      valueMaps[i][attribute.get()] = collection[i];
    }
  }
  
  if ( valueMaps.empty() ) {
    throw std::runtime_error("Token: no instances created for multi-instance activity '" + token->node->id +"'" );
  }

  if ( extensionElements->loopIndex.has_value() ) {
    // set value of loop index attribute for each instance
    if ( extensionElements->loopIndex.value()->attribute.has_value() ) {
      auto& attribute = extensionElements->loopIndex.value()->attribute.value();
      for ( size_t i = 0; i < valueMaps.size(); i++ ) {
        valueMaps[i][&attribute.get()] = i + 1;
      }
    }
    else {
      throw std::runtime_error("StateMachine: no attribute provided for loop index parameter of multi-instance activity '" + token->node->id +"'" );
    }
  }

  auto activity = token->node->represents<BPMN::Activity>();
  assert( activity );
  assert( activity->loopCharacteristics.has_value() );

  // create token copies
  assert ( systemState->tokensAtActivityInstance.find(token) == systemState->tokensAtActivityInstance.end() );
  assert ( systemState->exitStatusAtActivityInstance.find(token) == systemState->exitStatusAtActivityInstance.end() );

  Token* tokenCopy = nullptr;
  for ( auto valueMap : valueMaps ) {
    tokens.push_back( std::make_shared<Token>( token ) );
    // update status of token copy
    for ( auto [attribute,value] : valueMap ) {
      tokens.back().get()->status[attribute->index] = value;
    }

    if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::MultiInstanceSequential ) {
      if ( !tokenCopy ) {
        // for sequential multi-instance activities only the first token awaits entry event
        tokens.back().get()->update(Token::State::READY);
        tokens.back().get()->awaitEntryEvent();
      }
      else {
        // newly created tokens have to wait for previous token copy
        const_cast<SystemState*>(systemState)->tokenAwaitingExit[tokenCopy] = tokens.back().get();
      }
    }
    else if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::MultiInstanceParallel ) {
      // for parallel multi-instance activities all new tokens await entry event
      tokens.back().get()->update(Token::State::READY);
      tokens.back().get()->awaitEntryEvent();
    }

    tokenCopy = tokens.back().get();
    const_cast<SystemState*>(systemState)->tokensAtActivityInstance[token].push_back(tokenCopy);
    const_cast<SystemState*>(systemState)->exitStatusAtActivityInstance[token] = {};
    const_cast<SystemState*>(systemState)->tokenAtMultiInstanceActivity[tokenCopy] = token;
  }

  assert( tokenCopy );

  // change state of original token
  token->update(Token::State::WAITING);
}

void StateMachine::deleteMultiInstanceActivityToken(Token* token) {
  auto engine = const_cast<Engine*>(systemState->engine);
  auto mainToken = const_cast<SystemState*>(systemState)->tokenAtMultiInstanceActivity.at(token);

  auto activity = token->node->represents<BPMN::Activity>();
  assert( activity );

  if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::MultiInstanceSequential ) {
    // advance next token for sequential multi-instance activity
    auto& tokenAwaitingExit = const_cast<SystemState*>(systemState)->tokenAwaitingExit;
    auto it = tokenAwaitingExit.find(token);
  
    if ( it != tokenAwaitingExit.end() ) {
      auto waitingToken = it->second;
      waitingToken->awaitEntryEvent();
      tokenAwaitingExit.erase(it);
    }
  }

  // record exit status
  const_cast<SystemState*>(systemState)->exitStatusAtActivityInstance[mainToken].push_back(token->status);
  // remove token
  const_cast<SystemState*>(systemState)->tokenAtMultiInstanceActivity.erase(token);
  erase_ptr<Token>(const_cast<SystemState*>(systemState)->tokensAtActivityInstance[mainToken],token);
  erase_ptr<Token>(tokens,token);

  // advance main token when last multi-instance token exited
  auto& tokensAtActivityInstance = const_cast<SystemState*>(systemState)->tokensAtActivityInstance;
  if ( auto it = tokensAtActivityInstance.find(mainToken);
    it != tokensAtActivityInstance.end()
  ) {
    if ( it->second.empty() ) {
      // last multi-instance token exited
      tokensAtActivityInstance.erase(it);

      if ( !activity->boundaryEvents.empty() ) {
        // remove tokens at boundary events
        deleteTokensAwaitingBoundaryEvent( mainToken );
      }

      auto& exitStatusAtActivityInstance = const_cast<SystemState*>(systemState)->exitStatusAtActivityInstance;
      if ( auto it = exitStatusAtActivityInstance.find(mainToken);
         it != exitStatusAtActivityInstance.end()
      ) {
        // merge status 
        mainToken->status = BPMNOS::mergeValues(it->second);
        exitStatusAtActivityInstance.erase(it);
      }

      // advance main token
      if ( mainToken->node->outgoing.empty() ) {
        engine->commands.emplace_back(std::bind(&Token::advanceToDone,mainToken), mainToken);
      }
      else {
        engine->commands.emplace_back(std::bind(&Token::advanceToDeparting,mainToken), mainToken);
      }
    }
  }
  else {
    assert(!"cannot find tokens created for multi instance activity");
  }
}
// TODO: handle failures events
// TODO: handle single-instance compensations
// TODO: handle parallel compensations
// TODO: set value from csv input
// TODO: type vector (must be immutable)
// TODO: check conditions for immutable  

void StateMachine::deleteAdHocSubProcessToken(Token* token) {
  if ( tokens.size() > 1 ) {
    erase_ptr<Token>(tokens,token);
  }
  else {
    attemptShutdown();
  }
}

/*
void StateMachine::deleteChild(StateMachine* child) {
//std::cerr << "delete child '" << child->scope->id << "' of '" << scope->id << "'" <<  std::endl;
  if ( child->scope->represents<BPMN::SubProcess>() ) {
//    erase_ptr<StateMachine>(subProcesses, child); /// TODO: check
  }
  else {
    interruptingEventSubProcess.reset();
  }
}
*/

void StateMachine::deleteNonInterruptingEventSubProcess(StateMachine* eventSubProcess) {
//std::cerr << "deleteNonInterruptingEventSubProcess" << std::endl;
  erase_ptr<StateMachine>(nonInterruptingEventSubProcesses, eventSubProcess);
  attemptShutdown();
}

void StateMachine::deleteCompensationEventSubProcess(StateMachine* eventSubProcess) {
//std::cerr << compensationEventSubProcesses.size() << "deleteCompensationEventSubProcess: " << scope->id << "/" << eventSubProcess->scope->id << "/" << this <<  std::endl;
  erase_ptr<StateMachine>(compensationEventSubProcesses, eventSubProcess);
}

void StateMachine::clearObsoleteTokens() {
  for ( auto token : tokens ) {
    token->withdraw();
  }
  tokens.clear();
}

void StateMachine::interruptActivity(Token* token) {
//std::cerr << "interrupt activity " << token->node->id << std::endl;
  auto activity = token->node->represents<BPMN::Activity>();
  assert( activity );

  if ( activity->loopCharacteristics.has_value() &&
    activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::Standard
  ) {
    // withdraw all tokens for multi-instance activity
    auto it = const_cast<SystemState*>(systemState)->tokensAtActivityInstance.find(token);
    assert ( it != const_cast<SystemState*>(systemState)->tokensAtActivityInstance.end() );
    for ( auto activeToken : it->second ) {
//std::cerr << "withdraw token at " << activeToken->node->id << "/" << activeToken << std::endl;
      if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::MultiInstanceSequential ) {
        const_cast<SystemState*>(systemState)->tokenAwaitingExit.erase(activeToken);
      }

      const_cast<SystemState*>(systemState)->tokenAtMultiInstanceActivity.erase(activeToken);
      if ( activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::MultiInstanceParallel
        || activeToken->state != Token::State::READY
      ) {
        activeToken->withdraw();
      }
      erase_ptr<Token>(tokens, activeToken);
    }
    const_cast<SystemState*>(systemState)->tokensAtActivityInstance.erase(it);

    // remove all exit status for multi-instance activity
    const_cast<SystemState*>(systemState)->exitStatusAtActivityInstance.erase(token);
  }
  deleteTokensAwaitingBoundaryEvent(token);
  token->withdraw();
  erase_ptr<Token>(tokens, token);
}

void StateMachine::deleteTokensAwaitingBoundaryEvent(Token* token) {
//std::cerr << "deleteTokensAwaitingBoundaryEvent " << token->node->id << std::endl;
  // delete all tokens awaiting boundary event
  auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent;
  auto it = tokensAwaitingBoundaryEvent.find(token);
  if ( it != tokensAwaitingBoundaryEvent.end() ) {
    for ( auto waitingToken : it->second ) {
      waitingToken->withdraw();
      erase_ptr<Token>(tokens, waitingToken);
    }
    tokensAwaitingBoundaryEvent.erase(it);
  }
}

void StateMachine::registerRecipient() {
  if ( auto it = const_cast<SystemState*>(systemState)->unsent.find(instanceId);
    it != const_cast<SystemState*>(systemState)->unsent.end()
  ) {
    for ( auto& [message_ptr] : it->second ) {
      if ( auto message = message_ptr.lock() ) {
        const_cast<SystemState*>(systemState)->correspondence[this].emplace_back(message->weak_from_this());
        const_cast<SystemState*>(systemState)->outbox[message->origin].emplace_back(message->weak_from_this());
      }
    }
    const_cast<SystemState*>(systemState)->unsent.erase(it);
  }
}

void StateMachine::unregisterRecipient() {
  if ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>();
    !parentToken ||
    ( eventSubProcess && !eventSubProcess->startEvent->isInterrupting )
  ) {
    // delete all messages directed to state machine
    if ( auto it = const_cast<SystemState*>(systemState)->correspondence.find(this);
      it != const_cast<SystemState*>(systemState)->correspondence.end()
    ) {
      for ( auto& [message_ptr] : it->second ) {
        if ( auto message = message_ptr.lock() ) {
          erase_ptr(const_cast<SystemState*>(systemState)->messages, message.get());
        }
      }
      const_cast<SystemState*>(systemState)->correspondence.erase(it);
    }
  }
}

void StateMachine::run(const Values& status) {
  assert( status.size() >= 2 );
  assert( status[BPMNOS::Model::ExtensionElements::Index::Instance].has_value() );
  assert( status[BPMNOS::Model::ExtensionElements::Index::Timestamp].has_value() );

//std::cerr << "Run " << scope->id << "/" << this << "/" << parentToken << std::endl;
  if ( !parentToken ) {
    // state machine without parent token represents a process
//std::cerr << "Start process " << process->id << std::endl;
    tokens.push_back( std::make_shared<Token>(this,nullptr,status) );
    const_cast<std::string&>(instanceId) = BPMNOS::to_string(status[BPMNOS::Model::ExtensionElements::Index::Instance].value(),STRING);
    const_cast<SystemState*>(systemState)->archive[ instanceId ] = weak_from_this();
    registerRecipient();
  }
  else {
    for ( auto startNode : scope->startNodes ) {
      tokens.push_back( std::make_shared<Token>(this,startNode,status) );
    }
  }

  for ( auto token : tokens ) {
    if ( token->node ) {
      if ( auto startEvent = token->node->represents<BPMN::TypedStartEvent>();
        startEvent && !startEvent->isInterrupting
      ) {
        // token instantiates non-interrupting event subprocess
        // get instantiation counter from context
        auto context = const_cast<StateMachine*>(parentToken->owned.get());
        auto counter = ++context->instantiations[token->node];
        // append instantiation counter for disambiguation
        const_cast<std::string&>(instanceId) = BPMNOS::to_string(token->status[BPMNOS::Model::ExtensionElements::Index::Instance].value(),STRING) + delimiter +  std::to_string(counter);
        token->status[BPMNOS::Model::ExtensionElements::Index::Instance] = BPMNOS::to_number(instanceId,BPMNOS::ValueType::STRING);
        const_cast<SystemState*>(systemState)->archive[ instanceId ] = weak_from_this();
        registerRecipient();
      }
//std::cerr << "Initial token: >" << token->jsonify().dump() << "<" << std::endl;
    }
    // advance token
    token->advanceFromCreated();
  }
}

void StateMachine::createChild(Token* parent, const BPMN::Scope* scope) {
//std::cerr << "Create child from " << this << std::endl;
/// TODO: check
  parent->owned = std::make_shared<StateMachine>(systemState, scope, parent);
  parent->owned->run(parent->status);
}

void StateMachine::createInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status) {
  interruptingEventSubProcess = std::make_shared<StateMachine>(pendingEventSubProcess);
  interruptingEventSubProcess->run(status);
}

void StateMachine::createNonInterruptingEventSubprocess(const StateMachine* pendingEventSubProcess, const BPMNOS::Values& status) {
  nonInterruptingEventSubProcesses.push_back( std::make_shared<StateMachine>(pendingEventSubProcess) );
  nonInterruptingEventSubProcesses.back()->run(status);
}

void StateMachine::createCompensationTokenForBoundaryEvent(const BPMN::BoundaryEvent* compensateBoundaryEvent, BPMNOS::Values status) {
  std::shared_ptr<Token> compensationToken = std::make_shared<Token>(this,compensateBoundaryEvent, status);
  compensationToken->update(Token::State::BUSY);
  compensationTokens.push_back(std::move(compensationToken));
  
}

void StateMachine::createCompensationEventSubProcess(const BPMN::EventSubProcess* eventSubProcess, BPMNOS::Values status) {
//std::cerr << "createCompensationEventSubProcess: " << eventSubProcess->id << "/" << scope->id << "/" << parentToken->owner->scope->id <<std::endl;
  // create state machine for compensation event subprocess
  compensationEventSubProcesses.push_back(std::make_shared<StateMachine>(systemState, eventSubProcess, parentToken));
  // create token at start event of compensation event subprocess
  std::shared_ptr<Token> compensationToken = std::make_shared<Token>(compensationEventSubProcesses.back().get(), eventSubProcess->startEvent, status );
  compensationToken->update(Token::State::BUSY);
  compensationTokens.push_back(std::move(compensationToken));
}

void StateMachine::compensateActivity(Token* token) {
  auto compensationNode = token->node->as<BPMN::BoundaryEvent>()->attachedTo->compensatedBy;
//std::cerr << "compensationNode: " << compensationNode->id << std::endl;
  assert( compensationNode != nullptr );
  if ( auto compensationActivity = compensationNode->represents<BPMN::Activity>() ) {
    // move token to compensation activity
    token->node = compensationActivity;
    auto engine = const_cast<Engine*>(systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToEntered,token), token);
  }
}

std::vector<Token*> StateMachine::createTokenCopies(Token* token, const std::vector<BPMN::SequenceFlow*>& sequenceFlows) {
  std::vector<Token*> tokenCopies;
  // create a token copy for each new destination
  for ( [[maybe_unused]] auto _ : sequenceFlows ) {
    tokens.push_back( std::make_shared<Token>( token ) );
    tokenCopies.push_back(tokens.back().get());
  }

  // advance all token copies
  for (size_t i = 0; i < sequenceFlows.size(); i++ ) {
    auto tokenCopy = tokenCopies[i];
    auto engine = const_cast<Engine*>(systemState->engine);
    engine->commands.emplace_back(std::bind(&Token::advanceToDeparted,tokenCopy,sequenceFlows[i]), tokenCopy);
  }
  return tokenCopies;
}

void StateMachine::createMergedToken(const BPMN::FlowNode* gateway) {
  auto gatewayIt = const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation[this].find(gateway);
  auto& [key,arrivedTokens] = *gatewayIt;

  // create merged token
  std::shared_ptr<Token> mergedToken = std::make_shared<Token>(arrivedTokens);

  // remove tokens
  for ( auto arrivedToken : arrivedTokens ) {
    erase_ptr<Token>(tokens,arrivedToken);
  }
  const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation[this].erase(gatewayIt);

  // add merged token
  tokens.push_back(std::move(mergedToken));

  // advance merged token
  auto token = tokens.back().get();
  auto engine = const_cast<Engine*>(systemState->engine);
  engine->commands.emplace_back(std::bind(&Token::advanceToEntered,token), token);
}


void StateMachine::handleDivergingGateway(Token* token) {
  if ( token->node->represents<BPMN::ParallelGateway>() ) {
    // create token copies and advance them
    createTokenCopies(token, token->node->outgoing);
    // remove original token
    erase_ptr<Token>(tokens,token);
  }
  else if ( token->node->represents<BPMN::EventBasedGateway>() ) {
    // create token copies and advance them
    auto tokenCopies = createTokenCopies(token, token->node->outgoing);
    auto& tokenAtEventBasedGateway = const_cast<SystemState*>(systemState)->tokenAtEventBasedGateway;
    auto& tokensAwaitingEvent = const_cast<SystemState*>(systemState)->tokensAwaitingEvent;

    for ( auto tokenCopy : tokenCopies ) {
      tokenAtEventBasedGateway[tokenCopy] = token;
      tokensAwaitingEvent[token].push_back(tokenCopy);
    }
  }
  else {
    throw std::runtime_error("StateMachine: diverging gateway type not yet supported");
  }
}

void StateMachine::handleEventBasedGatewayActivation(Token* token) {
  auto tokenAtEventBasedGateway = const_cast<SystemState*>(systemState)->tokenAtEventBasedGateway.at(token);
  auto waitingTokens = const_cast<SystemState*>(systemState)->tokensAwaitingEvent.at(tokenAtEventBasedGateway);
  // remove all other waiting tokens
  for ( auto waitingToken : waitingTokens ) {
    if ( waitingToken != token ) {
      waitingToken->withdraw();
      erase_ptr<Token>(tokens,waitingToken);
    }
  }
  // remove token at event-based gateway
  tokenAtEventBasedGateway->update(Token::State::COMPLETED);
  erase_ptr<Token>(tokens,tokenAtEventBasedGateway);
}

void StateMachine::handleEscalation(Token* token) {
//std::cerr << "handleEscalation " << (token->node ? token->node->id : process->id ) << std::endl;
  if ( !parentToken ) {
    return;
  }

  auto it = std::find_if(pendingEventSubProcesses.begin(), pendingEventSubProcesses.end(), [](std::shared_ptr<StateMachine>& stateMachine) {
    auto eventSubProcess = stateMachine->scope->as<BPMN::EventSubProcess>();
    return eventSubProcess->startEvent->represents<BPMN::EscalationStartEvent>();
  });

  auto engine = const_cast<Engine*>(systemState->engine);

  if ( it != pendingEventSubProcesses.end() ) {
    // trigger event subprocess
    auto eventToken = it->get()->tokens.front().get();
//std::cerr << "found event-subprocess catching escalation:" << eventToken << "/" << eventToken->owner << std::endl;
    eventToken->setStatus(token->status);
    eventToken->advanceToCompleted();

    return;
  }

  if ( auto activity = token->node->represents<BPMN::Activity>();
    activity &&
    token->state != Token::State::WAITING &&
    activity->loopCharacteristics.has_value() &&
    activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::Standard
  ) {
    // handle failure at main token waiting for all instances
    auto mainToken = const_cast<SystemState*>(systemState)->tokenAtMultiInstanceActivity.at(token);
    mainToken->setStatus(token->status);
    handleEscalation(mainToken);
    return;
  }

  // find escalation boundary event
  if ( token->node ) {
    auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[token];
    for ( auto eventToken : tokensAwaitingBoundaryEvent) {
      if ( eventToken->node->represents<BPMN::EscalationBoundaryEvent>() ) {
        eventToken->setStatus(token->status);
        eventToken->advanceToCompleted();
        return;
      }
    }
  }

  // update status of parent token with that of current token
  parentToken->setStatus(token->status);
  parentToken->update(parentToken->state);

//std::cerr << "bubbble up escalation" << std::endl;
  auto parent = const_cast<StateMachine*>(parentToken->owner);
  engine->commands.emplace_back(std::bind(&StateMachine::handleEscalation,parent,parentToken), parentToken);

}

void StateMachine::handleFailure(Token* token) {
//std::cerr << scope->id << " handles failure at " << (token->node ? token->node->id : process->id ) << "/" << parentToken << "/" << token << std::endl;
  auto engine = const_cast<Engine*>(systemState->engine);

//  assert( !token->owned );

//std::cerr << "check whether failure is caught" << std::endl;

  if ( token->node ) {
    if ( auto activity = token->node->represents<BPMN::Activity>() ) {
      if ( activity->isForCompensation ) {
        // compensation activity failed, clear all other compensations
        compensationTokens.clear();
      }
      else if ( token->state != Token::State::WAITING &&
        activity->loopCharacteristics.has_value() &&
        activity->loopCharacteristics.value() != BPMN::Activity::LoopCharacteristics::Standard
      ) {
        // handle failure at main token waiting for all instances       
        auto mainToken = const_cast<SystemState*>(systemState)->tokenAtMultiInstanceActivity.at(token);
//std::cerr << "handle failure at main token waiting for all instances "  << std::endl;
        mainToken->setStatus(token->status);
        handleFailure(mainToken);
        return;
      }
    }
  }

  // find error boundary event at token node
  if ( token->node ) {
    auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[token];
    for ( auto eventToken : tokensAwaitingBoundaryEvent) {
      if ( eventToken->node->represents<BPMN::ErrorBoundaryEvent>() ) {
        eventToken->setStatus(token->status);
        eventToken->advanceToCompleted();
        return;
      }
    }
  }

  // find event-subprocess catching error
  auto it = std::find_if(pendingEventSubProcesses.begin(), pendingEventSubProcesses.end(), [](std::shared_ptr<StateMachine>& stateMachine) {
    auto eventSubProcess = stateMachine->scope->as<BPMN::EventSubProcess>();
    return eventSubProcess->startEvent->represents<BPMN::ErrorStartEvent>();
  });
  if ( it != pendingEventSubProcesses.end() ) {
    // trigger event subprocess
    auto eventToken = it->get()->tokens.front().get();
    // update status of event token with that of current token
    eventToken->setStatus(token->status);
    // remove all tokens
    clearObsoleteTokens();
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), eventToken);

    return;
  }

  // failure is not caught
//std::cerr << "uncaught failure" << std::endl;

  
  if ( !parentToken ) {
//std::cerr << "process has failed" << std::endl;
    // failure at process before state machine containing flow elements has been created
    assert(tokens.size() == 1);
    assert(token = tokens.front().get());
    engine->commands.emplace_back(std::bind(&Engine::deleteInstance,engine,this), this);
    return;
  }
  
  // bubble up error
  
  // update status of parent token with that of current token
  parentToken->setStatus(token->status);
  engine->commands.emplace_back(std::bind(&Token::advanceToFailed,parentToken), parentToken);
}

Token* StateMachine::findTokenAwaitingErrorBoundaryEvent(Token* activityToken) {
  auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[activityToken];
  for ( auto eventToken : tokensAwaitingBoundaryEvent) {
    if ( eventToken->node->represents<BPMN::ErrorBoundaryEvent>() ) {
      return eventToken;
    }
  }
  return nullptr;
}

void StateMachine::attemptGatewayActivation(const BPMN::FlowNode* node) {
  if ( node->represents<BPMN::ParallelGateway>() ) {

    auto gatewayIt = const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation[this].find(node);
    auto& [key,arrivedTokens] = *gatewayIt;
    if ( arrivedTokens.size() == node->incoming.size() ) {
      // create merged token and advance it
      auto engine = const_cast<Engine*>(systemState->engine);
      engine->commands.emplace_back(std::bind(&StateMachine::createMergedToken,this,node), this);
    }
  }
  else {
    throw std::runtime_error("StateMachine: converging gateway type not yet supported");
  }
}

void StateMachine::shutdown() {
  auto engine = const_cast<Engine*>(systemState->engine);
//std::cerr << "shutdown: " << scope->id << std::endl;
  assert( tokens.size() );
  BPMNOS::Values mergedStatus = Token::mergeStatus(tokens);
  
  if ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>() ) {
    // update global objective
    assert( scope->extensionElements->as<BPMNOS::Model::ExtensionElements>() );
    const_cast<SystemState*>(systemState)->objective += scope->extensionElements->as<BPMNOS::Model::ExtensionElements>()->getContributionToObjective(mergedStatus);

    if (!eventSubProcess->startEvent->isInterrupting ) {
      // delete non-interrupting event subprocess
      auto context = const_cast<StateMachine*>(parentToken->owned.get());
      engine->commands.emplace_back(std::bind(&StateMachine::deleteNonInterruptingEventSubProcess,context,this), this);
      return;
    }
    else if ( eventSubProcess->startEvent->represents<BPMN::CompensateStartEvent>() ) {
      completeCompensationEventSubProcess();
      return;
    }
  }

//std::cerr << "start shutdown: " << scope->id << std::endl;

  // update status of parent token
  if ( parentToken ) {
    parentToken->setStatus(mergedStatus);
  }
  tokens.clear();

  // ensure that messages to state machine are removed  
  unregisterRecipient();

  if ( auto subProcess = scope->represents<BPMN::SubProcess>();
    subProcess && subProcess->compensatedBy
  ) {
    if ( subProcess->compensatedBy->represents<BPMN::EventSubProcess>() ) {
      auto parent = const_cast<StateMachine*>(parentToken->owner);
      // move state machine pointer to ensure it is not deleted
      parent->compensableSubProcesses.push_back(shared_from_this());
    }
  }

  if ( !parentToken ) {
    // delete root state machine (and all descendants)
    engine->commands.emplace_back(std::bind(&Engine::deleteInstance,engine,this), this);
  }
  else {
//std::cerr << "delete child: " << scope->id << std::endl;
    auto parent = const_cast<StateMachine*>(parentToken->owner);
    if ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>();
      eventSubProcess && eventSubProcess->startEvent->isInterrupting
    ) {
      parent->interruptingEventSubProcess.reset();
    }
//    engine->commands.emplace_back(std::bind(&StateMachine::deleteChild,parent,this), this);

    // advance parent token to completed
    auto context = const_cast<StateMachine*>(parentToken->owned.get());
    auto token = context->parentToken;
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,token), token);
  }

//std::cerr << "shutdown (done): " << scope->id <<std::endl;
}

void StateMachine::attemptShutdown() {
//std::cerr << "attemptShutdown: " << scope->id << "/" << this << std::endl;
  if ( interruptingEventSubProcess ) {
//std::cerr << "wait for completion of interrupting event subprocess" << interruptingEventSubProcess->scope->id << std::endl;
    // wait for completion of interrupting event subprocess
    return;
  }

  if ( nonInterruptingEventSubProcesses.size() ) {
    // wait until last event subprocess is completed
    return;
  }

  for ( auto& token : tokens ) {
    if ( token->state != Token::State::DONE ) {
      return;
    }
  }

  // all tokens are in DONE state

  // no new event subprocesses can be triggered
  for ( auto eventSubProcess : pendingEventSubProcesses ) {
    eventSubProcess->clearObsoleteTokens();
  }
  pendingEventSubProcesses.clear();

  auto engine = const_cast<Engine*>(systemState->engine);
//std::cerr << "Shutdown with " << engine->commands.size() << " prior commands" << std::endl;
  engine->commands.emplace_back(std::bind(&StateMachine::shutdown,this), this);
}

Tokens StateMachine::getCompensationTokens(const BPMN::Activity* activity) const {
//std::cerr << scope->id << " getCompensationTokens of " << ( activity ? activity->id : std::string("all activities") )  << std::endl; 
  Tokens result;
  if ( compensationTokens.empty() ) {
//std::cerr << "no compensation token" << std::endl;
    return result;
  }

  if ( activity ) {
    // find compensation within context
    auto it = std::find_if(
       compensationTokens.begin(),
       compensationTokens.end(),
       [&activity](const std::shared_ptr<Token>& token) -> bool {
         // check if compensation token is at compensation boundary event
         return ( token.get()->node->as<BPMN::BoundaryEvent>()->attachedTo == activity );
       }
    );
    if ( it != compensationTokens.end() ) {
      result.push_back(*it);
    }
  }
  else {
    result = compensationTokens;
  }

//std::cerr << "compensating " << result.size() << " tokens" << std::endl;
  return result;
}


void StateMachine::advanceTokenWaitingForCompensation(Token* waitingToken) {
//std::cerr << scope->id << " -> advanceTokenWaitingForCompensation: " << waitingToken->node->id << "/" << Token::stateName[(int)waitingToken->state]<< "/" << waitingToken << std::endl;
  auto engine = const_cast<Engine*>(systemState->engine);
  if ( waitingToken->state == Token::State::BUSY ) {

    // erase from compensation tokens and move to tokens
    if ( waitingToken->node->represents<BPMN::CompensateBoundaryEvent>() ) {
      tokens.emplace_back(waitingToken->shared_from_this());
      erase_ptr<Token>(compensationTokens,waitingToken);
    }
    else if ( waitingToken->node->represents<BPMN::CompensateStartEvent>() ) {
      const_cast<StateMachine*>(waitingToken->owner)->tokens.emplace_back(waitingToken->shared_from_this());
      erase_ptr<Token>(compensationTokens,waitingToken);
    } 

//std::cerr << "Continue with advanceToCompleted: " << waitingToken->node->id << std::endl;
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,waitingToken), waitingToken);
  }
  else if ( waitingToken->state == Token::State::FAILING ) {
//std::cerr << "Continue with terminate: " << waitingToken->owner->scope->id << std::endl;
    assert( waitingToken->owner == this);
//    engine->commands.emplace_back(std::bind(&StateMachine::terminate,this), this);
    engine->commands.emplace_back(std::bind(&StateMachine::handleFailure,this,waitingToken), waitingToken);
  }
  else {
//  std::cerr << waitingToken->node->id << " has state: " << Token::stateName[(int)waitingToken->state]  << std::endl;
    assert(!"unexpected state of waiting token");
  }
}

void StateMachine::completeCompensationActivity(Token* token) {
  // token is still the compensation token

  // advance waiting token
  auto& tokenAwaitingCompensationActivity = const_cast<SystemState*>(systemState)->tokenAwaitingCompensationActivity;
  auto waitingToken = tokenAwaitingCompensationActivity.at(token);
  const_cast<StateMachine*>(waitingToken->owner)->advanceTokenWaitingForCompensation( waitingToken );
  tokenAwaitingCompensationActivity.erase(token);

  // remove token
//std::cerr << scope->id  << " -> Compensation completed: " << token->node->id << "/" << token << "/" << tokens.size() << "/" << compensationTokens.size() << std::endl;
  erase_ptr<Token>(tokens,token);
}

void StateMachine::completeCompensationEventSubProcess() {
  auto& tokenAwaitingCompensationEventSubProcess = const_cast<SystemState*>(systemState)->tokenAwaitingCompensationEventSubProcess;
  auto waitingToken = tokenAwaitingCompensationEventSubProcess.at(this);
  const_cast<StateMachine*>(waitingToken->owner)->advanceTokenWaitingForCompensation( waitingToken );
  tokenAwaitingCompensationEventSubProcess.erase(this);
  auto engine = const_cast<Engine*>(systemState->engine);
  // erase state machine
  auto context = const_cast<StateMachine*>(parentToken->owned.get());
  engine->commands.emplace_back(std::bind(&StateMachine::deleteCompensationEventSubProcess,context,this), this);
}


void StateMachine::compensate(Tokens compensations, Token* waitingToken) {
//std::cerr << "\ncompensate: " << scope->id << " compensate " <<  compensations.size() << " tokens before continuing with " << waitingToken->node->id << std::endl;

  auto engine = const_cast<Engine*>(systemState->engine);
  auto& tokenAwaitingCompensationActivity = const_cast<SystemState*>(systemState)->tokenAwaitingCompensationActivity;
  auto& tokenAwaitingCompensationEventSubProcess = const_cast<SystemState*>(systemState)->tokenAwaitingCompensationEventSubProcess;

  auto it = compensations.rbegin();
  // advance last compensation token
  auto compensationToken = it->get();

  if ( compensationToken->node->represents<BPMN::CompensateStartEvent>() ) {
    const_cast<StateMachine*>(compensationToken->owner)->tokens.push_back(std::move(*it));
  }
  else {
    tokens.push_back(std::move(*it));
  }
//std::cerr << engine->commands.size() <<" Compensate " << compensationToken->node->id << "/scope: " << compensationToken->owner->scope->id << std::endl;

  // erase compensation token and move to active tokens
  erase_ptr<Token>(compensationTokens,compensationToken);

  engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,compensationToken), compensationToken);

  // go to next compensation token
  it++;
  while ( it != compensations.rend() ) {
//std::cerr << engine->commands.size() <<"+Compensate " << compensationToken->node->id << "/scope: " << compensationToken->owner->scope->id << std::endl;
    // create awaiters for all other compensations
    if ( compensationToken->node->represents<BPMN::CompensateStartEvent>() ) {
      tokenAwaitingCompensationEventSubProcess[const_cast<StateMachine*>(compensationToken->owner)] = it->get();
    }
    else {
      tokenAwaitingCompensationActivity[compensationToken] = it->get();
    }
    compensationToken = it->get();
    it++;
  }
  // create awaiter for waiting token
  if ( compensationToken->node->represents<BPMN::CompensateStartEvent>() ) {
    tokenAwaitingCompensationEventSubProcess[const_cast<StateMachine*>(compensationToken->owner)] = waitingToken;
  }
  else {
    tokenAwaitingCompensationActivity[compensationToken] = waitingToken;
  }
}

