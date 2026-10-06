#include "StaticDataProvider.h"
#include "InstanceDataReader.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/StateMachine.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/events/ClockTickEvent.h"
#include "execution/engine/src/events/InstantiationEvent.h"
#include "execution/engine/src/events/ReadyEvent.h"
#include "execution/engine/src/events/CompletionEvent.h"
#include "execution/engine/src/events/TerminationEvent.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/bpmnos/src/extensionElements/Expression.h"
#include "model/utility/src/CollectionRegistry.h"
#include <cmath>
#include <stdexcept>

using namespace BPMNOS::Execution;

StaticDataProvider::Scenario::Scenario(std::shared_ptr<const StaticDataProvider> dataProvider)
  : Execution::Scenario(std::move(dataProvider))
{
}

StaticDataProvider::StaticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, unsigned int clockTickDuration)
  : StaticDataProvider(std::move(model), clockTickDuration)
{
  readInstances(instanceFileOrString, { "INSTANCE_ID", "NODE_ID", "INITIALIZATION" }, sharedModel->limexHandle);
}

StaticDataProvider::StaticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, unsigned int clockTickDuration)
  : DataProvider(model.get(), clockTickDuration)
  , sharedModel(std::move(model))
{
}

void StaticDataProvider::readInstances(const std::string& instanceFileOrString, const std::vector<std::string>& columns, const LIMEX::Handle<double>& handle) {
  InstanceDataReader reader(sharedModel.get(), instanceFileOrString, columns);
  for ( auto& row : reader.rows ) {
    if ( !row.node ) {
      reader.evaluateGlobal(row.initialization, handle);
    }
    else if ( !row.initialization.empty() ) {
      auto [attribute, expression] = reader.lookupAttribute(row.node, row.initialization);
      reader.setValue(row.instanceId, attribute, reader.evaluate(row.instanceId, row.node, expression, attribute->type, handle));
    }
  }

  // the globals given are completed by those the model assigns
  globals = BPMNOS::Values(sharedModel->attributes.size());
  for ( auto& [attribute, value] : reader.globals ) {
    globals[attribute->index] = value;
  }
  for ( auto& attribute : sharedModel->attributes ) {
    if ( attribute->expression && !globals[attribute->index].has_value() ) {
      auto value = attribute->expression->execute(BPMNOS::Values{}, BPMNOS::Values{}, globals);
      if ( !value.has_value() ) {
        throw std::runtime_error("StaticDataProvider: failed to evaluate global attribute '" + attribute->id + "'");
      }
      globals[attribute->index] = value.value();
    }
  }

  for ( auto& [instanceId, process] : reader.processes ) {
    auto values = reader.values.contains(instanceId) ? reader.values.at(instanceId) : std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>{};
    reader.addDefaultValues(instanceId, values);
    auto timestampAttribute = process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].get();
    // instances are instantiated at integral times
    auto instantiationTime = BPMNOS::number(std::ceil((double)values.at(timestampAttribute)));
    earliestInstantiationTime = std::min(earliestInstantiationTime, instantiationTime);
    instances[instanceId] = Instance{process, std::move(values), instantiationTime};
  }
}

std::unique_ptr<StaticDataProvider::Scenario> StaticDataProvider::createScenario() const {
  return std::make_unique<Scenario>(std::static_pointer_cast<const StaticDataProvider>(shared_from_this()));
}

void StaticDataProvider::setEndTime(BPMNOS::number endTime) {
  this->endTime = endTime;
}

BPMNOS::number StaticDataProvider::getEndTime() const {
  return endTime;
}

BPMNOS::Values StaticDataProvider::getGlobals([[maybe_unused]] const Execution::Scenario& scenario) const {
  return globals;
}

BPMNOS::number StaticDataProvider::getEarliestInstantiationTime([[maybe_unused]] const Execution::Scenario& scenario) const {
  return earliestInstantiationTime;
}

void StaticDataProvider::notice(const Observable* observable, Execution::Scenario& executionScenario, EventQueue& queue) const {
  auto& scenario = static_cast<Scenario&>(executionScenario);

  if ( observable->getObservableType() == Observable::Type::SystemState ) {
    // an installed state holds every instance known up to its time; the events it awaits are determined anew
    auto systemState = static_cast<const SystemState*>(observable);
    scenario.scheduledEvents.clear();
    for ( auto& [token_ptr] : systemState->tokensAwaitingReadyEvent ) {
      if ( auto token = token_ptr.lock() ) {
        if ( token->node ) {
          readyActivity(scenario, queue, token.get());
        }
        else {
          readyProcess(scenario, queue, token.get());
        }
      }
    }
    for ( auto element : systemState->tokensAwaitingCompletionEvent ) {
      if ( auto token = std::get<1>(element).lock() ) {
        completeTask(scenario, queue, token.get());
      }
    }
    return;
  }

  if ( observable->getObservableType() == Observable::Type::Event ) {
    if ( auto clockTickEvent = static_cast<const Event*>(observable)->is<ClockTickEvent>() ) {
      if ( clockTickEvent->systemState->getTime() == std::numeric_limits<BPMNOS::number>::lowest() ) {
        // the first clock tick event of a run
        instantiate(scenario, queue, clockTickEvent->time);
      }
      // the events due by the time the clock advances to are released in the order of their times
      auto end = scenario.scheduledEvents.upper_bound(clockTickEvent->time);
      for ( auto it = scenario.scheduledEvents.begin(); it != end; ++it ) {
        queue.push_back(it->second);
      }
      scenario.scheduledEvents.erase(scenario.scheduledEvents.begin(), end);
    }
    return;
  }

  if ( observable->getObservableType() != Observable::Type::Token ) {
    return;
  }
  auto token = static_cast<const Token*>(observable);

  if ( !token->node ) {
    if ( token->state == Token::State::CREATED ) {
      readyProcess(scenario, queue, token);
    }
    return;
  }

  // the instances of a multi-instance activity receive no ready event of their own, being created from the
  // ready event of the main token, which is never a key of tokenAtMultiInstanceActivity
  if (
    token->node->represents<BPMN::Activity>() &&
    ( token->state == Token::State::ARRIVED || token->state == Token::State::CREATED ) &&
    !token->owner->systemState->tokenAtMultiInstanceActivity.contains(const_cast<Token*>(token))
  ) {
    readyActivity(scenario, queue, token);
    return;
  }

  if (
    token->node->represents<BPMN::Task>() &&
    token->state == Token::State::BUSY &&
    // send, receive and decision tasks complete through other events
    !token->node->represents<BPMN::SendTask>() &&
    !token->node->represents<BPMN::ReceiveTask>() &&
    !token->node->represents<BPMNOS::Model::DecisionTask>()
  ) {
    completeTask(scenario, queue, token);
  }
}

void StaticDataProvider::dispatchEvent([[maybe_unused]] const SystemState* systemState, [[maybe_unused]] Execution::Scenario& scenario, [[maybe_unused]] EventQueue& queue) const {
  // every event is enqueued when the notification giving rise to it is noticed
}

void StaticDataProvider::advance(const SystemState* systemState, Execution::Scenario& executionScenario, EventQueue& queue) const {
  auto& scenario = static_cast<Scenario&>(executionScenario);
  if (
    systemState->getTime() >= endTime ||
    ( systemState->instances.empty() && scenario.scheduledEvents.empty() && isExhausted(scenario, systemState->getTime()) )
  ) {
    queue.push_back(std::make_shared<TerminationEvent>());
    return;
  }
  DataProvider::advance(systemState, scenario, queue);
}

void StaticDataProvider::instantiate([[maybe_unused]] Scenario& scenario, EventQueue& queue, [[maybe_unused]] BPMNOS::number time) const {
  for ( auto& [instanceId, instance] : instances ) {
    queue.push_back(std::make_shared<InstantiationEvent>(instance.process, getStatus(instanceId, instance.process), getData(instanceId, instance.process)));
  }
}

void StaticDataProvider::readyProcess(Scenario& scenario, EventQueue& queue, const Token* token) const {
  auto instanceId = (size_t)token->owner->root->instance.value();
  auto& instance = instances.at(instanceId);
  auto event = std::make_shared<ReadyEvent>(token, getStatus(instanceId, instance.process), getData(instanceId, instance.process));
  schedule(scenario, queue, instance.instantiationTime, token->owner->systemState->getTime(), std::move(event));
}

void StaticDataProvider::readyActivity([[maybe_unused]] Scenario& scenario, EventQueue& queue, const Token* token) const {
  // the values of the activity are given for the instance of the process the token belongs to
  auto instanceId = (size_t)token->owner->root->instance.value();
  auto status = token->status;
  for ( auto value : getStatus(instanceId, token->node) ) {
    status.push_back(value);
  }
  queue.push_back(std::make_shared<ReadyEvent>(token, std::move(status), getData(instanceId, token->node)));
}

void StaticDataProvider::completeTask(Scenario& scenario, EventQueue& queue, const Token* token) const {
  auto dueTime = token->status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value();
  schedule(scenario, queue, dueTime, token->owner->systemState->getTime(), std::make_shared<CompletionEvent>(token, token->status));
}

bool StaticDataProvider::isExhausted([[maybe_unused]] const Scenario& scenario, [[maybe_unused]] BPMNOS::number time) const {
  return true;
}

void StaticDataProvider::schedule(Scenario& scenario, EventQueue& queue, BPMNOS::number dueTime, BPMNOS::number currentTime, std::shared_ptr<Event> event) {
  if ( dueTime <= currentTime ) {
    queue.push_back(std::move(event));
  }
  else {
    scenario.scheduledEvents.emplace(dueTime, std::move(event));
  }
}

std::optional<BPMNOS::number> StaticDataProvider::getValue(size_t instanceId, const BPMNOS::Model::Attribute* attribute) const {
  if ( attribute->expression && attribute->expression->type == BPMNOS::Model::Expression::Type::ASSIGN ) {
    // the value is computed from values of the instance which must not change during the run
    std::vector<double> variableValues;
    for ( auto input : attribute->expression->variables ) {
      auto value = input->isImmutable ? getValue(instanceId, input) : std::nullopt;
      if ( !value.has_value() ) {
        return std::nullopt;
      }
      variableValues.push_back((double)value.value());
    }
    std::vector<std::vector<double>> collectionValues;
    for ( auto input : attribute->expression->collections ) {
      auto collection = input->isImmutable ? getValue(instanceId, input) : std::nullopt;
      if ( !collection.has_value() ) {
        return std::nullopt;
      }
      collectionValues.emplace_back(collectionRegistry[(size_t)collection.value()].begin(), collectionRegistry[(size_t)collection.value()].end());
    }
    return BPMNOS::number(attribute->expression->compiled.evaluate(variableValues, collectionValues));
  }
  auto& values = instances.at(instanceId).values;
  if ( auto it = values.find(attribute); it != values.end() ) {
    return it->second;
  }
  return std::nullopt;
}

BPMNOS::Values StaticDataProvider::getStatus(size_t instanceId, const BPMN::Node* node) const {
  BPMNOS::Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    result.push_back(getValue(instanceId, attribute.get()));
  }
  return result;
}

BPMNOS::Values StaticDataProvider::getData(size_t instanceId, const BPMN::Node* node) const {
  BPMNOS::Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    result.push_back(getValue(instanceId, attribute.get()));
  }
  return result;
}
