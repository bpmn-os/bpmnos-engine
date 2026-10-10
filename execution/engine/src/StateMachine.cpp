#include "Engine.h"
#include "model/bpmnos/src/Model.h"
#include "StateMachine.h"
#include "Token.h"
#include "SystemState.h"
#include "Message.h"
#include "Event.h"
#include "execution/utility/src/erase.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Timer.h"
#include "model/bpmnos/src/extensionElements/SignalDefinition.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/SequentialAdHocSubProcess.h"
#include "bpmn++.h"
#include <cassert>
#include <ranges>

using namespace BPMNOS::Execution;

StateMachine::StateMachine(const SystemState* systemState, Data globals)
  : systemState(systemState)
  , scope(nullptr)
  , root(nullptr)
  , parentToken(nullptr)
  , ownedData(std::move(globals))
  , data(SharedData(ownedData))
{
}

Token* StateMachine::createInstance(const BPMN::Process* process, Data data, Status status) {
  assert( !parentToken );
  assert( data.attributes.size() >= 1 );
  assert( data.attributes[BPMNOS::Model::ExtensionElements::Position::Instance].has_value() );
  assert( status.attributes.size() >= 1 );
  assert( status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].has_value() );
  // the token at the process holds the status; it is advanced by the caller, since advancing notifies
  // observers
  auto token = tokens.emplace_back( std::make_shared<Token>(this,process,std::move(status)) ).get();
  // the state machine of the instance holds its data from the instantiation on, so that the token at the
  // process carries the data before the instance is started
  auto instance = data.attributes[BPMNOS::Model::ExtensionElements::Position::Instance];
  createChild(token, process, std::move(data), instance);
  token->data = &token->owned->data;
  return token;
}

StateMachine::StateMachine(const SystemState* systemState, const BPMN::Scope* scope, Token* parentToken, Data dataAttributes, std::optional<BPMNOS::number> instance )
  : systemState(systemState)
  , scope(scope)
  , root(scope && scope->represents<BPMN::EventSubProcess>() ? parentToken->owned->root : parentToken->owner->root ? parentToken->owner->root : this)
  , instance(instance.has_value() ? instance : (*parentToken->data).attributes[systemState->engine->getModel()->instanceIndex].get())
  , parentToken(parentToken)
  , ownedData(dataAttributes)
  , data(SharedData(scope && scope->represents<BPMN::EventSubProcess>() ? parentToken->owned->data : parentToken->owner->data,ownedData))
{
  
  data.attributes[systemState->engine->getModel()->instanceIndex] = std::ref(this->instance);
/*
std::cerr << "child StateMachine(" << scope->id << "/" << this << " @ " << parentToken << ")" << " owned by: " << parentToken->owner << std::endl;
std::cerr << "data: ";
for ( auto& attribute : data ) {
std::cerr << (int)attribute.get().value() << ", ";
}
std::cerr << std::endl;
*/
  assert( this->instance.has_value() && this->instance.value() >= 0 );

  if ( scope && !scope->represents<BPMN::EventSubProcess>() && !scope->represents<BPMN::Process>() ) {
    // an event subprocess is instantiated when its start event is triggered and not when the state machine
    // awaiting the trigger is created, so its data is accounted there; a pending event subprocess that is
    // never triggered instantiates nothing and accounts nothing. The data of a process instance is accounted
    // when the instance is started, since the data may change until then
    updateObjective();
  }
}

StateMachine::StateMachine(const StateMachine* other)
  : systemState(other->systemState)
  , scope(other->scope)
  , root(other->root)
  , instance( other->instance )
  , parentToken(other->parentToken)
  , ownedData(other->ownedData)
  , data(SharedData(parentToken->owned->data,ownedData))
{
//std::cerr << "oStateMachine(" << scope->id << "/" << this << " @ " << parentToken << ")"  << " owned by :" << parentToken->owner << std::endl;
  data.attributes[systemState->engine->getModel()->instanceIndex] = std::ref(instance);
}

StateMachine::StateMachine(const SystemState* systemState, Token* parentToken, const StateMachine* other, const StateMachine* context)
  : systemState(systemState)
  , scope(other->scope)
  , root(!parentToken ? nullptr : ( context ? context : parentToken->owner )->root ? ( context ? context : parentToken->owner )->root : this)
  , instance(other->instance)
  , parentToken(parentToken)
  , ownedData(other->ownedData)
  , data(parentToken ? SharedData(( context ? context : parentToken->owner )->data, ownedData) : SharedData(ownedData))
  , instantiations(other->instantiations)
{
  if ( parentToken ) {
    // the global state machine has no instance
    data.attributes[systemState->engine->getModel()->instanceIndex] = std::ref(instance);
  }

  // Copy tokens
  for (const auto& otherToken : other->tokens) {
    tokens.push_back(std::make_shared<Token>(const_cast<StateMachine*>(this), otherToken.get()));

    // Populate pending*Decisions containers (after make_shared, needs weak_from_this)
    auto& token = tokens.back();
    if (token->decisionRequest) {
      auto type = token->decisionRequest->type;

      if (type == Observable::Type::EntryRequest) {
        const_cast<SystemState*>(systemState)->pendingEntryDecisions.emplace_back(token, token->decisionRequest);
      }
      else if (type == Observable::Type::ChoiceRequest) {
        const_cast<SystemState*>(systemState)->pendingChoiceDecisions.emplace_back(token, token->decisionRequest);
      }
      else if (type == Observable::Type::ExitRequest) {
        const_cast<SystemState*>(systemState)->pendingExitDecisions.emplace_back(token, token->decisionRequest);
      }
      else if (type == Observable::Type::MessageDeliveryRequest) {
        // the kind has just been read, so the request is of that kind and carries the header with it
        const_cast<SystemState*>(systemState)->pendingMessageDeliveryDecisions.emplace_back(
          token, std::static_pointer_cast<MessageDeliveryRequest>(token->decisionRequest));
      }
    }

    // Populate tokensAwaitingTimer
    if (
      token->node->represents<BPMN::TimerCatchEvent>() && 
      token->state == Token::State::BUSY
    ) {
      assert(other->systemState->tokensAwaitingTimer.find(otherToken.get()) != other->systemState->tokensAwaitingTimer.end());
      auto trigger = token->node->extensionElements->as<BPMNOS::Model::Timer>()->trigger.get();
      BPMNOS::number time = trigger->expression->execute(token->status, *token->data).value();
      const_cast<SystemState*>(systemState)->tokensAwaitingTimer.emplace(time, token);
    }

    // Populate tokensAwaitingSignal
    if (
      token->node->represents<BPMN::SignalCatchEvent>() && 
      token->state == Token::State::BUSY
    ) {
      auto signalName = token->node->extensionElements->as<BPMNOS::Model::SignalDefinition>()->name;
      assert(other->systemState->tokensAwaitingSignal.at(signalName).find(otherToken.get()) != other->systemState->tokensAwaitingSignal.at(signalName).end());
      const_cast<SystemState*>(systemState)->tokensAwaitingSignal[signalName].emplace_back(token);
    }

    // Populate tokensAwaitingCondition
    if (
      token->node->represents<BPMN::ConditionalCatchEvent>() && 
      token->state == Token::State::BUSY
    ) {
      assert(other->systemState->tokensAwaitingCondition.at(other->root->instance.value()).find(otherToken.get()) != other->systemState->tokensAwaitingCondition.at(other->root->instance.value()).end());
      const_cast<SystemState*>(systemState)->tokensAwaitingCondition[root->instance.value()].emplace_back(token);
    }

    // Populate tokensAwaitingReadyEvent
    // Multi-instance instance tokens may also sit in CREATED state (sequential copies awaiting
    // their predecessor), but they receive no ready event of their own and are instead
    // reconstructed via the multi-instance containers below; exclude them here.
    // The token at a process instance that is created but not yet started awaits its ready event too.
    if (
      (
        token->node->represents<BPMN::Activity>() &&
        (token->state == Token::State::CREATED || token->state == Token::State::ARRIVED) &&
        !other->systemState->tokenAtMultiInstanceActivity.contains(otherToken.get())
      ) ||
      ( token->node->represents<BPMN::Process>() && token->state == Token::State::CREATED )
    ) {
      assert(other->systemState->tokensAwaitingReadyEvent.find(otherToken.get()) != other->systemState->tokensAwaitingReadyEvent.end());
      const_cast<SystemState*>(systemState)->tokensAwaitingReadyEvent.emplace_back(token);
    }

    // Populate tokensAwaitingCompletionEvent
    if (
      token->node->represents<BPMN::Task>() &&
      !token->node->represents<BPMN::ReceiveTask>() &&
      !token->node->represents<BPMN::SendTask>() &&
      !token->node->represents<BPMNOS::Model::DecisionTask>() &&
      token->state == Token::State::BUSY
    ) {
      assert(other->systemState->tokensAwaitingCompletionEvent.find(otherToken.get()) != other->systemState->tokensAwaitingCompletionEvent.end());
      auto time = token->status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].value();
      const_cast<SystemState*>(systemState)->tokensAwaitingCompletionEvent.emplace(time, token);
    }

    // Create message for SendTask tokens awaiting delivery
    // (outbox, unsent, inbox are populated at the end of SystemState copy constructor)
    if (
      token->node->represents<BPMN::SendTask>() &&
      token->state == Token::State::BUSY
    ) {
      assert(other->systemState->messageAwaitingDelivery.contains(otherToken.get()));
      auto otherMessage = other->systemState->messageAwaitingDelivery.at(otherToken.get()).lock();
      assert(otherMessage);
      auto message = std::make_shared<Message>(otherMessage.get(), token.get());
      auto newSystemState = const_cast<SystemState*>(systemState);
      newSystemState->messages.push_back(message);
      newSystemState->messageAwaitingDelivery[token.get()] = message;
    }

    // Populate boundary event containers
    if (
      token->node->represents<BPMN::BoundaryEvent>() &&
      !token->node->represents<BPMN::CompensateBoundaryEvent>() &&
      token->state == Token::State::BUSY
    ) {
      auto it = other->systemState->tokenAssociatedToBoundaryEventToken.find(otherToken.get());
      assert(it != other->systemState->tokenAssociatedToBoundaryEventToken.end());
      const BPMN::Node* activityNode = it->second->node;

      auto activityIt = std::ranges::find_if(tokens, [activityNode](const auto& t) { return t->node == activityNode; });
      assert(activityIt != tokens.end());
      Token* activityToken = activityIt->get();

      const_cast<SystemState*>(systemState)->tokenAssociatedToBoundaryEventToken[token.get()] = activityToken;
      const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[activityToken].push_back(token.get());
    }

    // Populate event-based gateway containers
    if (
      token->node->represents<BPMN::CatchEvent>() &&
      token->state == Token::State::BUSY &&
      !token->node->as<BPMN::FlowNode>()->incoming.empty() &&
      token->node->as<BPMN::FlowNode>()->incoming.front()->source->represents<BPMN::EventBasedGateway>()
    ) {
      assert(other->systemState->tokenAtEventBasedGateway.find(otherToken.get()) !=
             other->systemState->tokenAtEventBasedGateway.end());
      const BPMN::FlowNode* gatewayNode = token->node->as<BPMN::FlowNode>()->incoming.front()->source;

      auto gatewayIt = std::ranges::find_if(tokens, [gatewayNode](const auto& t) { return t->node == gatewayNode; });
      assert(gatewayIt != tokens.end());
      Token* gatewayToken = gatewayIt->get();

      const_cast<SystemState*>(systemState)->tokenAtEventBasedGateway[token.get()] = gatewayToken;
      const_cast<SystemState*>(systemState)->tokensAwaitingEvent[gatewayToken].push_back(token.get());
    }

    // Populate tokensAwaitingGatewayActivation (converging gateway)
    if (
      token->sequenceFlow &&
      token->state == Token::State::WAITING &&
      token->node->represents<BPMN::Gateway>() &&
      !token->node->represents<BPMN::ExclusiveGateway>() &&
      token->node->as<BPMN::FlowNode>()->incoming.size() > 1
    ) {
      assert(other->systemState->tokensAwaitingGatewayActivation.contains(const_cast<StateMachine*>(other)));
      assert(other->systemState->tokensAwaitingGatewayActivation.at(const_cast<StateMachine*>(other)).contains(otherToken->node->as<BPMN::FlowNode>()));
      assert(std::ranges::contains(other->systemState->tokensAwaitingGatewayActivation.at(const_cast<StateMachine*>(other)).at(otherToken->node->as<BPMN::FlowNode>()), otherToken.get()));

      const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation[const_cast<StateMachine*>(this)][token->node->as<BPMN::FlowNode>()].push_back(token.get());
    }

    // Populate multi-instance containers (tokenAtMultiInstanceActivity & tokensAtActivityInstance)
    if (other->systemState->tokenAtMultiInstanceActivity.contains(otherToken.get())) {
      // This is an activity instance token, find main token by node + WAITING state
      auto mainIt = std::ranges::find_if(tokens, [&](const auto& t) {
        return t->node == token->node && t->state == Token::State::WAITING;
      });
      assert(mainIt != tokens.end());
      Token* mainToken = mainIt->get();

      const_cast<SystemState*>(systemState)->tokenAtMultiInstanceActivity[token.get()] = mainToken;
      const_cast<SystemState*>(systemState)->tokensAtActivityInstance[mainToken].push_back(token.get());
    }

    // Populate exitStatusAtActivityInstance (main token)
    if (other->systemState->exitStatusAtActivityInstance.contains(otherToken.get())) {
      const_cast<SystemState*>(systemState)->exitStatusAtActivityInstance[token.get()] = other->systemState->exitStatusAtActivityInstance.at(otherToken.get());
    }

    // Populate performing and pendingSequentialEntries for activities in SequentialAdHocSubProcess
    if (
      token->node->represents<BPMN::Activity>() &&
      token->node->as<BPMN::FlowNode>()->parent->represents<BPMNOS::Model::SequentialAdHocSubProcess>()
    ) {
      Token* performerToken = token->getSequentialPerformerToken();
      Token* otherPerformerToken = otherToken->getSequentialPerformerToken();

      if (otherPerformerToken->performing == otherToken.get()) {
        performerToken->performing = token.get();
      }

      if (otherPerformerToken->pendingSequentialEntries.find(otherToken.get()) != otherPerformerToken->pendingSequentialEntries.end()) {
        performerToken->pendingSequentialEntries.emplace_back(token);
      }
    }
  }

  // Populate tokenAwaitingMultiInstanceExit (sequential only) - after all tokens copied
  for (auto& [otherPrevToken, otherNextToken] : other->systemState->tokenAwaitingMultiInstanceExit) {
    if (otherPrevToken->owner != other) continue;  // Only process tokens owned by this StateMachine

    // Match by node and instance identity.
    auto prevMatch = [&](const auto& token) {
      return token->node == otherPrevToken->node && token->getInstanceId() == otherPrevToken->getInstanceId();
    };
    auto nextMatch = [&](const auto& token) {
      return token->node == otherNextToken->node && token->getInstanceId() == otherNextToken->getInstanceId();
    };
    auto prevIt = std::ranges::find_if(tokens, prevMatch);
    auto nextIt = std::ranges::find_if(tokens, nextMatch);
    assert(prevIt != tokens.end() && nextIt != tokens.end());

    const_cast<SystemState*>(systemState)->tokenAwaitingMultiInstanceExit[prevIt->get()] = nextIt->get();
  }

  // Copy compensationTokens (at CompensateBoundaryEvent or CompensateStartEvent in BUSY state)
  for (const auto& otherToken : other->compensationTokens) {
    compensationTokens.push_back(std::make_shared<Token>(const_cast<StateMachine*>(this), otherToken.get()));
  }

  // Populate tokenAwaitingCompensationActivity - after tokens and compensationTokens copied
  auto findTokenByNode = [&](const BPMN::Node* node) -> Token* {
    for (const auto& token : tokens) {
      if (token->node == node) return token.get();
    }
    for (const auto& token : compensationTokens) {
      if (token->node == node) return token.get();
    }
    return nullptr;
  };

  for (const auto& [otherCompensationToken, otherAwaitingToken] : other->systemState->tokenAwaitingCompensationActivity) {
    if (otherCompensationToken->owner != other) continue;

    Token* compensationToken = findTokenByNode(otherCompensationToken->node);
    Token* awaitingToken = findTokenByNode(otherAwaitingToken->node);
    assert(compensationToken && awaitingToken);

    const_cast<SystemState*>(systemState)->tokenAwaitingCompensationActivity[compensationToken] = awaitingToken;
  }

  // Copy event subprocesses (pending, interrupting, non-interrupting)
  for (const auto& otherEventSubProcess : other->pendingEventSubProcesses) {
    pendingEventSubProcesses.push_back(std::make_shared<StateMachine>(systemState, parentToken, otherEventSubProcess.get(), this));
  }
  if (other->interruptingEventSubProcess) {
    interruptingEventSubProcess = std::make_shared<StateMachine>(systemState, parentToken, other->interruptingEventSubProcess.get(), this);
  }
  for (const auto& otherEventSubProcess : other->nonInterruptingEventSubProcesses) {
    nonInterruptingEventSubProcesses.push_back(std::make_shared<StateMachine>(systemState, parentToken, otherEventSubProcess.get(), this));
  }

  // Copy compensableSubProcesses (tokens owning compensable subprocesses)
  for (const auto& otherToken : other->compensableSubProcesses) {
    compensableSubProcesses.push_back(std::make_shared<Token>(const_cast<StateMachine*>(this), otherToken.get()));
  }

  // Copy compensationEventSubProcesses and populate tokenAwaitingCompensationEventSubProcess
  for (const auto& otherEventSubProcess : other->compensationEventSubProcesses) {
    compensationEventSubProcesses.push_back(std::make_shared<StateMachine>(systemState, parentToken, otherEventSubProcess.get(), this));

    // Populate tokenAwaitingCompensationEventSubProcess if original had a waiting token
    if (auto it = other->systemState->tokenAwaitingCompensationEventSubProcess.find(otherEventSubProcess.get());
        it != other->systemState->tokenAwaitingCompensationEventSubProcess.end()) {
      Token* waitingToken = findTokenByNode(it->second->node);
      assert(waitingToken);
      const_cast<SystemState*>(systemState)->tokenAwaitingCompensationEventSubProcess[compensationEventSubProcesses.back().get()] = waitingToken;
    }
  }
}

StateMachine::~StateMachine() {
//std::cerr << "~StateMachine()" << std::endl;
  const_cast<SystemState*>(systemState)->tokensAwaitingGatewayActivation.erase(this);
  const_cast<SystemState*>(systemState)->tokenAwaitingCompensationEventSubProcess.erase(this);
  if ( !systemState->inbox.empty() && scope ) {
    unregisterRecipient();
  }
  compensationTokens.clear();
}

BPMNOS::Data StateMachine::getData(const BPMN::Scope* scope) {
  throw std::runtime_error("StateMachine: data is not yet known for scope '" + scope->id + "'");
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
  tokens.push_back( std::make_shared<Token>(this,node,token->status) );
  auto createdToken = tokens.back().get();
  const_cast<SystemState*>(systemState)->tokenAssociatedToBoundaryEventToken[createdToken] = token;
  const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[token].push_back(createdToken);
  createdToken->advanceToEntered();
}

void StateMachine::takeTriggeringStatus(Token* eventToken, const Status& status) {
  // the token triggering an event subprocess belongs to an enclosing scope, so the status it hands over
  // holds values for the attributes of the enclosing scopes only, possibly followed by values of a nested
  // scope it was raised in; the status of the start token keeps its layout, the attributes of the event
  // subprocess remaining undefined until the model assigns values to them when it is triggered
  auto extensionElements = eventToken->node->as<BPMN::FlowNode>()->parent->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  auto ownAttributes = extensionElements ? extensionElements->attributes.size() : 0;
  auto statusSize = eventToken->status.attributes.size();
  eventToken->status = status;
  eventToken->status.attributes.resize( statusSize - ownAttributes );
  eventToken->status.attributes.resize( statusSize );
}

BPMNOS::Data StateMachine::undefinedData(const BPMN::Node* node) {
  auto extensionElements = node->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  return Data( extensionElements ? extensionElements->data.size() : 0 );
}

void StateMachine::initiateEventSubprocesses(Token* token) {
//std::cerr << "initiate " << scope->eventSubProcesses.size() << " eventSubprocesses for token at " << token->node->id << "/" << parentToken << "/" << token << " owned by " << token->owner << std::endl;
  for ( auto& eventSubProcess : scope->eventSubProcesses ) {
    // the data of an event subprocess is internal: it is undefined until the model assigns values to it
    // when the event subprocess is triggered
    pendingEventSubProcesses.push_back(std::make_shared<StateMachine>( systemState, eventSubProcess, parentToken, undefinedData(eventSubProcess) ) );
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
    auto getLoopCardinality = [token,extensionElements]() -> std::optional<BPMNOS::number> {
      if (!extensionElements->loopCardinality.value()->expression) {
        return std::nullopt;
      }
      return BPMNOS::to_value( extensionElements->loopCardinality.value()->expression->execute(token->status, *token->data) );
    };
    // use provided cardinality to determine number of tokens 
    if ( auto loopCardinality = getLoopCardinality();
      loopCardinality.has_value()
    ) {
      valueMaps.resize( (size_t)loopCardinality.value() );
    }
    else {
      throw std::runtime_error("StateMachine: cannot determine cardinality for multi-instance activity '" + token->node->id +"'" );
    }
  }
  
  if ( valueMaps.empty() ) {
    throw std::runtime_error("StateMachine: no cardinality provided for multi-instance activity '" + token->node->id +"'" );
  }

  if ( extensionElements->loopIndex.has_value() && extensionElements->loopIndex.value()->expression ) {
    // set value of loop index attribute for each instance
    if ( auto attribute = extensionElements->loopIndex.value()->expression->isAttribute() ) {
      for ( size_t i = 0; i < valueMaps.size(); i++ ) {
        valueMaps[i][attribute] = i + 1;
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
  size_t counter = 0;
  for ( auto valueMap : valueMaps ) {
    counter++;
    // disambiguate instance id
    auto instanceId = BPMNOS::to_string(this->data.attributes[systemState->engine->getModel()->instanceIndex].get().value(),STRING) + StateMachine::delimiters[0] + token->node->id + StateMachine::delimiters[1] +  std::to_string(counter);

    tokens.push_back( std::make_shared<Token>( token ) );
    if ( auto scope = token->node->represents<BPMN::Scope>() ) {
      // create state machine for each multi-instance subprocess with the data the main token received with
      // its ready event
      assert( token->owned );
      Data data = token->owned->ownedData;

      // create child state machine with disambiguated instance identifier
      createChild( tokens.back().get(), scope, std::move(data), BPMNOS::to_number(instanceId,BPMNOS::ValueType::STRING) );
      // ensure that data is set appropriately
      tokens.back()->data = &tokens.back()->owned->data;
//std::cerr << "MI:" << instanceId << "/" << tokens.back()->jsonify() << std::endl;      
    }
    else {
      // create child fake state machine with disambiguated instance identifier
      createChild( tokens.back().get(), nullptr, {}, BPMNOS::to_number(instanceId,BPMNOS::ValueType::STRING) );
      // ensure that data is set appropriately
      tokens.back()->data = &tokens.back()->owned->data;
//std::cerr << "Fake:" << tokens.back()->jsonify() << std::endl;      
    }       
    
    // update status of token copy
    for ( auto [attribute,value] : valueMap ) {
      tokens.back().get()->status.attributes[attribute->index] = value;
    }

    auto instanceToken = tokens.back().get();

    // Register instance bookkeeping BEFORE any state notification, so that the ready-event
    // handler recognises this as a multi-instance instance token and skips it (the single
    // ready event is consumed by the main token, not re-raised for each instance).
    const_cast<SystemState*>(systemState)->tokensAtActivityInstance[token].push_back(instanceToken);
    const_cast<SystemState*>(systemState)->exitStatusAtActivityInstance[token] = {};
    const_cast<SystemState*>(systemState)->tokenAtMultiInstanceActivity[instanceToken] = token;

    // Every instance emits its own CREATED state (birth); ARRIVED is skipped (no incoming flow).
    instanceToken->update(Token::State::CREATED);

    if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::MultiInstanceSequential ) {
      if ( !tokenCopy ) {
        // for sequential multi-instance activities only the first instance becomes ready immediately
        instanceToken->update(Token::State::READY);
//std::cerr << "Token awaiting entry:" << instanceToken->jsonify() << std::endl;
        instanceToken->awaitEntryEvent();
      }
      else {
        // subsequent instances remain in CREATED until their predecessor has exited
        const_cast<SystemState*>(systemState)->tokenAwaitingMultiInstanceExit[tokenCopy] = instanceToken;
      }
    }
    else if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::MultiInstanceParallel ) {
      // for parallel multi-instance activities all instances become ready immediately
      instanceToken->update(Token::State::READY);
//std::cerr << "Token awaiting entry:" << instanceToken->jsonify() << std::endl;
      instanceToken->awaitEntryEvent();
    }

    tokenCopy = instanceToken;
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

  // Instance has no outgoing flow: emit DONE as its terminal state before removal.
  // Aggregation into the main token is handled below, not via the generic scope shutdown.
  token->update(Token::State::DONE);

  if ( activity->loopCharacteristics.value() == BPMN::Activity::LoopCharacteristics::MultiInstanceSequential ) {
    // advance next token for sequential multi-instance activity
    auto& tokenAwaitingMultiInstanceExit = const_cast<SystemState*>(systemState)->tokenAwaitingMultiInstanceExit;
    auto it = tokenAwaitingMultiInstanceExit.find(token);
  
    if ( it != tokenAwaitingMultiInstanceExit.end() ) {
      auto waitingToken = it->second;
      // successor leaves CREATED and becomes ready now that its predecessor has exited
      waitingToken->update(Token::State::READY);
      waitingToken->awaitEntryEvent();
      tokenAwaitingMultiInstanceExit.erase(it);
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
  if ( auto it1 = tokensAtActivityInstance.find(mainToken);
    it1 != tokensAtActivityInstance.end()
  ) {
    if ( it1->second.empty() ) {
      // last multi-instance token exited
      tokensAtActivityInstance.erase(it1);

      if ( !activity->boundaryEvents.empty() ) {
        // remove tokens at boundary events
        deleteTokensAwaitingBoundaryEvent( mainToken );
      }

      auto& exitStatusAtActivityInstance = const_cast<SystemState*>(systemState)->exitStatusAtActivityInstance;
      if ( auto it2 = exitStatusAtActivityInstance.find(mainToken);
         it2 != exitStatusAtActivityInstance.end()
      ) {
        // merge status 
        mainToken->status = BPMNOS::mergeStatus(it2->second);
        exitStatusAtActivityInstance.erase(it2);
      }

      // advance main token
      if ( mainToken->node->as<BPMN::FlowNode>()->outgoing.empty() ) {
        engine->commands.emplace_back(std::bind(&Token::advanceToDone,mainToken), mainToken);
      }
      else {
        engine->commands.emplace_back(std::bind(&Token::advanceToDeparting,mainToken), mainToken);
      }
    }
  }
  else {
    assert(false && "cannot find tokens created for multi instance activity");
  }
}
// TODO: handle failure events
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
        const_cast<SystemState*>(systemState)->tokenAwaitingMultiInstanceExit.erase(activeToken);
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
  if ( auto it = const_cast<SystemState*>(systemState)->unsent.find((long unsigned int)instance.value());
    it != const_cast<SystemState*>(systemState)->unsent.end()
  ) {
    for ( auto& [message_ptr] : it->second ) {
      if ( auto message = message_ptr.lock() ) {
        const_cast<SystemState*>(systemState)->inbox[this].emplace_back(message->weak_from_this());
        const_cast<SystemState*>(systemState)->outbox[message->origin].emplace_back(message->weak_from_this());
      }
    }
    const_cast<SystemState*>(systemState)->unsent.erase(it);
  }
}

void StateMachine::unregisterRecipient() {
//std::cerr << "unregisterRecipient" << std::endl;
  // the messages directed to a process instance are withdrawn when the instance is deleted
  if ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>();
    eventSubProcess && !eventSubProcess->startEvent->isInterrupting
  ) {
    withdrawMessages();
  }
}

void StateMachine::withdrawMessages() {
  // delete all messages directed to state machine
  if ( auto it = const_cast<SystemState*>(systemState)->inbox.find(this);
    it != const_cast<SystemState*>(systemState)->inbox.end()
  ) {
    for ( auto& [message_ptr] : it->second ) {
      if ( auto message = message_ptr.lock() ) {
        // withdraw message
        message->state = Message::State::WITHDRAWN;
        systemState->engine->notify(message.get());
        erase_ptr(const_cast<SystemState*>(systemState)->messages, message.get());
      }
    }
    const_cast<SystemState*>(systemState)->inbox.erase(it);
  }
}


void StateMachine::run(Status status) {
  // the state machine of a process creates its token on construction
  assert( parentToken );
  assert( status.attributes.size() >= 1 );
  assert( data.attributes.size() >= 1 );
  assert( data.attributes[systemState->engine->getModel()->instanceIndex].get().has_value() );
  assert( status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].has_value() );

//std::cerr << "Run " << scope->id << "/" << this << "/" << parentToken << std::endl;
  for ( auto startNode : scope->startNodes ) {
    tokens.push_back( std::make_shared<Token>(this,startNode,std::move(status)) );
  }

  for ( auto token : tokens ) {
    if ( auto flowNode = token->node->represents<BPMN::FlowNode>() ) {
      // a typed start event of a process belongs to an instance already created by the trigger, with
      // the status and the identifier it was created with
      if ( auto startEvent = token->node->represents<BPMN::TypedStartEvent>();
        startEvent && flowNode->parent->represents<BPMN::EventSubProcess>()
      ) {
        // the status attributes of the event subprocess are internal: they are undefined until the model
        // assigns values to them when the event subprocess is triggered
        auto extensionElements = flowNode->parent->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
        token->status.attributes.resize( token->status.attributes.size() + ( extensionElements ? extensionElements->attributes.size() : 0 ) );

        if ( !startEvent->isInterrupting ) {
          // token instantiates non-interrupting event subprocess
          // get instantiation counter from context
          auto context = const_cast<StateMachine*>(parentToken->owned.get());
          auto counter = ++context->instantiations[flowNode];
          // disambiguate instance id
          auto instanceId = BPMNOS::to_string((*parentToken->data).attributes[systemState->engine->getModel()->instanceIndex].get().value(),STRING) + StateMachine::delimiters[0] + scope->id + StateMachine::delimiters[1] + std::to_string(counter);
          data.attributes[systemState->engine->getModel()->instanceIndex].get() = BPMNOS::to_number(instanceId,BPMNOS::ValueType::STRING);
          const_cast<SystemState*>(systemState)->archive[ (long unsigned int)data.attributes[systemState->engine->getModel()->instanceIndex].get().value() ] = weak_from_this();
          registerRecipient();
        }
      }
//std::cerr << "Initial token: >" << token->jsonify().dump() << "<" << std::endl;
    }
    // advance token
    token->advanceFromCreated();
  }
}

void StateMachine::updateObjective() {
  auto extensionElements = scope->extensionElements->represents<BPMNOS::Model::ExtensionElements>();
  if ( !extensionElements ) {
    return;
  }


  // only the data this state machine owns is accounted here. The data of an enclosing scope is reachable
  // through @ref data but was accounted by the state machine owning it, and the owned values are the tail
  // of @ref data, everything before them belonging to an ancestor.
  size_t firstOwned = data.attributes.size() - ownedData.attributes.size();

  BPMNOS::number DELTA = 0;
  for ( auto& attribute : extensionElements->data ) {
    if ( attribute->weight != 0 && attribute->index >= firstOwned ) {
      if ( auto value = data.attributes[attribute->index].get(); value.has_value() ) {
        DELTA += value.value() * attribute->weight;
      }
    }
  }

  const_cast<Engine*>(systemState->engine)->addToObjective(DELTA);
}

void StateMachine::createChild(Token* parent, const BPMN::Scope* scope, Data data, std::optional<BPMNOS::number> instance) {
//std::cerr << "Create child from " << this << std::endl;
  parent->owned = std::make_shared<StateMachine>(systemState, scope, parent, std::move(data), instance.has_value() ? instance : (*parent->data).attributes[systemState->engine->getModel()->instanceIndex].get() );
//  parent->owned->run(parent->status);
}

void StateMachine::createCompensationTokenForBoundaryEvent(const BPMN::BoundaryEvent* compensateBoundaryEvent, BPMNOS::Status status) {
  std::shared_ptr<Token> compensationToken = std::make_shared<Token>(this,compensateBoundaryEvent, status);
  compensationToken->update(Token::State::BUSY);
  compensationTokens.push_back(std::move(compensationToken));
}

void StateMachine::createCompensationEventSubProcess(const BPMN::EventSubProcess* eventSubProcess, BPMNOS::Status status) {
//std::cerr << "createCompensationEventSubProcess: " << eventSubProcess->id << "/" << scope->id << "/" << parentToken->owner->scope->id <<std::endl;
  // create state machine for compensation event subprocess, whose data is internal and undefined until the
  // model assigns values to it when its start event completes
  compensationEventSubProcesses.push_back(std::make_shared<StateMachine>( systemState, eventSubProcess, parentToken, undefinedData(eventSubProcess) ) );
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
    if ( auto scope = token->node->represents<BPMN::Scope>() ) {
      // the data of a compensation activity is internal: it is undefined until the model assigns values to
      // it when the activity is entered
      createChild( token, scope, undefinedData(scope) );
    }
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
    createTokenCopies(token, token->node->as<BPMN::FlowNode>()->outgoing);
    // remove original token
    erase_ptr<Token>(tokens,token);
  }
  else if ( token->node->represents<BPMN::EventBasedGateway>() ) {
    // create token copies and advance them
    auto tokenCopies = createTokenCopies(token, token->node->as<BPMN::FlowNode>()->outgoing);
    auto& tokenAtEventBasedGateway = const_cast<SystemState*>(systemState)->tokenAtEventBasedGateway;
    auto& tokensAwaitingEvent = const_cast<SystemState*>(systemState)->tokensAwaitingEvent;

    for ( auto tokenCopy : tokenCopies ) {
      tokenAtEventBasedGateway[tokenCopy] = token;
      tokensAwaitingEvent[token].push_back(tokenCopy);
    }
  }
  else {
    throw std::runtime_error("StateMachine: diverging gateway type of node '" + token->node->id + "' not yet supported");
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
//std::cerr << "handleEscalation " << token->node->id << std::endl;
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
    takeTriggeringStatus(eventToken, token->status);
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
    mainToken->status = token->status;
    handleEscalation(mainToken);
    return;
  }

  // find escalation boundary event
  if ( token->node->represents<BPMN::FlowNode>() ) {
    auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[token];
    for ( auto eventToken : tokensAwaitingBoundaryEvent) {
      if ( eventToken->node->represents<BPMN::EscalationBoundaryEvent>() ) {
        eventToken->status = token->status;
        eventToken->advanceToCompleted();
        return;
      }
    }
  }

  // update status of parent token with that of current token
  parentToken->status = token->status;
  parentToken->update(parentToken->state);

//std::cerr << "bubbble up escalation" << std::endl;
  auto parent = const_cast<StateMachine*>(parentToken->owner);
  engine->commands.emplace_back(std::bind(&StateMachine::handleEscalation,parent,parentToken), parentToken);

}

void StateMachine::handleFailure(Token* token) {
//std::cerr << scope->id << " handles failure at " << token->node->id << "/" << parentToken << "/" << token << std::endl;
  auto engine = const_cast<Engine*>(systemState->engine);

//  assert( !token->owned );

//std::cerr << "check whether failure is caught" << std::endl;

  if ( token->node->represents<BPMN::FlowNode>() ) {
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
        mainToken->status = token->status;
        handleFailure(mainToken);
        return;
      }
    }
  }

  // find error boundary event at token node
  if ( token->node->represents<BPMN::FlowNode>() ) {
    auto& tokensAwaitingBoundaryEvent = const_cast<SystemState*>(systemState)->tokensAwaitingBoundaryEvent[token];
    for ( auto eventToken : tokensAwaitingBoundaryEvent) {
      if ( eventToken->node->represents<BPMN::ErrorBoundaryEvent>() ) {
        eventToken->status = token->status;
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
    takeTriggeringStatus(eventToken, token->status);
    // remove all tokens
    clearObsoleteTokens();
    engine->commands.emplace_back(std::bind(&Token::advanceToCompleted,eventToken), eventToken);

    return;
  }

  // failure is not caught
//std::cerr << "uncaught failure" << std::endl;

  
  if ( !parentToken ) {
//std::cerr << "process has failed" << std::endl;
    // the token at the process of a failed instance is removed from the global state machine
    engine->commands.emplace_back(std::bind(&Engine::deleteInstance,engine,token), token);
    return;
  }
  
  // bubble up error
  
  // update status of parent token with that of current token
  parentToken->status = token->status;
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
    throw std::runtime_error("StateMachine: converging gateway type of node '" + node->id + "' not yet supported");
  }
}

void StateMachine::shutdown() {
  auto engine = const_cast<Engine*>(systemState->engine);
//std::cerr << "shutdown: " << scope->id << std::endl;
  assert( tokens.size() );
  BPMNOS::Status mergedStatus = Token::mergeStatus(tokens);
  
  if ( auto eventSubProcess = scope->represents<BPMN::EventSubProcess>() ) {
    // the status attributes declared for the event subprocess are accounted where it shuts down
    auto extensionElements = scope->extensionElements->as<BPMNOS::Model::ExtensionElements>();
    assert( extensionElements );
    BPMNOS::number DELTA = 0;
    for ( auto& attribute : extensionElements->attributes ) {
      if ( attribute->weight != 0 ) {
        if ( auto value = extensionElements->attributeRegistry.getValue(attribute.get(),mergedStatus,data); value.has_value() ) {
          DELTA += value.value() * attribute->weight;
        }
      }
    }
    engine->addToObjective(DELTA);

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

//std::cerr << "start shutdown: " << BPMNOS::to_string(instance.value(),STRING) << std::endl;

  // update status of parent token (if it doesn't have a sequential performer)
  if ( parentToken &&
    !scope->extensionElements->as<BPMNOS::Model::ExtensionElements>()->hasSequentialPerformer
  ) {
    parentToken->status = mergedStatus;
  }
  tokens.clear();

  // ensure that messages to state machine are removed  
  unregisterRecipient();
  
  // the global state machine never shuts down, the tokens of completed instances being removed from it
  assert( parentToken );
  auto parent = const_cast<StateMachine*>(parentToken->owner);

//std::cerr << "delete child: " << scope->id << std::endl;
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

  if ( auto subProcess = scope->represents<BPMN::SubProcess>();
    subProcess && subProcess->compensatedBy
  ) {
    if ( subProcess->compensatedBy->represents<BPMN::EventSubProcess>() ) {
      // create token copy to own the compensable subprocess
      auto tokenCopy = std::make_shared<Token>(parentToken);
      tokenCopy->owned = shared_from_this();
      parentToken = tokenCopy.get();
      parent->compensableSubProcesses.push_back(std::move(tokenCopy));
    }
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

  if ( nonInterruptingEventSubProcesses.size() ) {
    // wait until last event subprocess is completed
    return;
  }

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
    assert(false && "unexpected state of waiting token");
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

