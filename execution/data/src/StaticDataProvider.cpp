#include "StaticDataProvider.h"
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

StaticDataProvider::StaticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, std::chrono::milliseconds clockTickDuration)
  : StaticDataProvider(std::move(model), clockTickDuration)
{
  readInstances(instanceFileOrString, { "INSTANCE_ID", "NODE_ID", "INITIALIZATION" }, this->model->limexHandle);
}

StaticDataProvider::StaticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, std::chrono::milliseconds clockTickDuration)
  : DataProvider(std::move(model), clockTickDuration)
{
}

void StaticDataProvider::readInstances(const std::string& instanceFileOrString, const std::vector<std::string>& columns, const LIMEX::Handle<double>& handle) {
  InstanceDataReader reader(this->model.get(), instanceFileOrString, columns);
  for ( auto& row : reader.rows ) {
    if ( !row.node ) {
      reader.evaluateGlobal(row.initialization, handle);
    }
    else {
      readValue(reader, row, handle);
    }
  }

  // the globals given are completed by those the model assigns
  globals = BPMNOS::Values(this->model->attributes.size());
  for ( auto& [attribute, value] : reader.globals ) {
    globals[attribute->index] = value;
  }
  for ( auto& attribute : this->model->attributes ) {
    if ( attribute->expression && !globals[attribute->index].has_value() ) {
      auto value = attribute->expression->execute(BPMNOS::Values{}, BPMNOS::Values{}, globals);
      if ( !value.has_value() ) {
        throw std::runtime_error("StaticDataProvider: failed to evaluate global attribute '" + attribute->id + "'");
      }
      globals[attribute->index] = value.value();
    }
  }

  // the instances are kept in the order of their first rows
  for ( auto& row : reader.rows ) {
    if ( !row.node || instancePositions.contains(row.instanceId) ) {
      continue;
    }
    auto instanceId = row.instanceId;
    auto process = reader.processes.at(instanceId);
    auto values = reader.values.contains(instanceId) ? reader.values.at(instanceId) : std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>{};
    reader.addDefaultValues(instanceId, values);
    auto timestampAttribute = process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].get();
    // instances are instantiated at integral times
    auto instantiationTime = BPMNOS::number(std::ceil((double)values.at(timestampAttribute)));
    instancePositions[instanceId] = instances.size();
    instances.push_back(Instance{instanceId, process, std::move(values), instantiationTime});
  }
}

void StaticDataProvider::readValue(InstanceDataReader& reader, const InstanceDataReader::Row& row, const LIMEX::Handle<double>& handle) {
  if ( !row.initialization.empty() ) {
    auto [attribute, expression] = reader.lookupAttribute(row.node, row.initialization);
    reader.setValue(row.instanceId, attribute, reader.evaluate(row.instanceId, row.node, expression, attribute->type, handle));
  }
}

std::unique_ptr<BPMNOS::Execution::Scenario> StaticDataProvider::createScenario([[maybe_unused]] unsigned int realisation) const {
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

BPMNOS::number StaticDataProvider::getEarliestInstantiationTime(const Execution::Scenario& scenario) const {
  auto earliestInstantiationTime = std::numeric_limits<BPMNOS::number>::max();
  for ( auto& instance : instances ) {
    earliestInstantiationTime = std::min(earliestInstantiationTime, getProcessReadyTime(static_cast<const Scenario&>(scenario), instance.id));
  }
  return earliestInstantiationTime;
}

void StaticDataProvider::notice(const Observable* observable, Execution::Scenario& executionScenario, EventQueue& queue) const {
  auto& scenario = static_cast<Scenario&>(executionScenario);

  if ( observable->getObservableType() == Observable::Type::SystemState ) {
    // the state of a run not yet begun holds no instance, an installed state every instance known up to its
    // time; the instances not yet held are instantiated when they become known, and the events the tokens
    // await are determined anew
    auto systemState = static_cast<const SystemState*>(observable);
    scenario.time = systemState->getTime();
    bool runBegun = ( systemState->getTime() != std::numeric_limits<BPMNOS::number>::lowest() );
    scenario.scheduledEvents.clear();
    for ( auto& instance : instances ) {
      if ( !runBegun || getKnownTime(scenario, instance.id) > systemState->getTime() ) {
        auto event = std::make_shared<InstantiationEvent>(instance.process, getStatus(scenario, instance.id, instance.process), getData(scenario, instance.id, instance.process));
        schedule(scenario, queue, getKnownTime(scenario, instance.id), systemState->getTime(), std::move(event));
      }
    }
    for ( auto& [token_ptr] : systemState->tokensAwaitingReadyEvent ) {
      if ( auto token = token_ptr.lock() ) {
        if ( token->node->represents<BPMN::FlowNode>() ) {
          readyActivity(scenario, queue, token.get(), systemState->getTime());
        }
        else {
          readyProcess(scenario, queue, token.get());
        }
      }
    }
    for ( auto element : systemState->tokensAwaitingCompletionEvent ) {
      if ( auto token = std::get<1>(element).lock() ) {
        completeTask(scenario, queue, token.get(), systemState->getTime());
      }
    }
    return;
  }

  if ( observable->getObservableType() == Observable::Type::Event ) {
    if ( auto clockTickEvent = static_cast<const Event*>(observable)->is<ClockTickEvent>() ) {
      scenario.time = clockTickEvent->time;
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

  if ( token->node->represents<BPMN::Process>() ) {
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
    readyActivity(scenario, queue, token, std::numeric_limits<BPMNOS::number>::lowest());
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
    completeTask(scenario, queue, token, std::numeric_limits<BPMNOS::number>::lowest());
  }
}

void StaticDataProvider::dispatchEvent([[maybe_unused]] const SystemState* systemState, [[maybe_unused]] Execution::Scenario& scenario, [[maybe_unused]] EventQueue& queue) const {
  // every event is enqueued when the notification giving rise to it is noticed
}

void StaticDataProvider::advance(const SystemState* systemState, Execution::Scenario& executionScenario, EventQueue& queue) const {
  auto& scenario = static_cast<Scenario&>(executionScenario);
  if (
    systemState->getTime() >= endTime ||
    ( systemState->globalStateMachine->tokens.empty() && scenario.scheduledEvents.empty() )
  ) {
    queue.push_back(std::make_shared<TerminationEvent>());
    return;
  }
  DataProvider::advance(systemState, scenario, queue);
}

const StaticDataProvider::Instance& StaticDataProvider::getInstance(size_t instanceId) const {
  return instances[instancePositions.at(instanceId)];
}

const std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>& StaticDataProvider::getInstanceValues([[maybe_unused]] const Scenario& scenario, size_t instanceId) const {
  return getInstance(instanceId).values;
}

BPMNOS::number StaticDataProvider::getKnownTime([[maybe_unused]] const Scenario& scenario, [[maybe_unused]] size_t instanceId) const {
  return std::numeric_limits<BPMNOS::number>::lowest();
}

BPMNOS::number StaticDataProvider::getProcessReadyTime([[maybe_unused]] const Scenario& scenario, size_t instanceId) const {
  return getInstance(instanceId).instantiationTime;
}

BPMNOS::Values StaticDataProvider::getActivityReadyStatus(Scenario& scenario, const Token* token, [[maybe_unused]] BPMNOS::number earliest) const {
  // the values of the activity are given for the instance of the process the token belongs to
  auto status = token->status;
  for ( auto value : getStatus(scenario, (size_t)token->owner->root->instance.value(), token->node) ) {
    status.push_back(value);
  }
  return status;
}

BPMNOS::number StaticDataProvider::getActivityReadyTime([[maybe_unused]] const Scenario& scenario, [[maybe_unused]] size_t instanceId, [[maybe_unused]] const BPMN::Node* activity, [[maybe_unused]] const BPMNOS::Values& readyStatus) const {
  return std::numeric_limits<BPMNOS::number>::lowest();
}

BPMNOS::Values StaticDataProvider::getCompletionStatus([[maybe_unused]] Scenario& scenario, const Token* token, [[maybe_unused]] BPMNOS::number earliest) const {
  return token->status;
}

void StaticDataProvider::readyProcess(Scenario& scenario, EventQueue& queue, const Token* token) const {
  auto instanceId = (size_t)token->getInstanceId();
  auto process = getInstance(instanceId).process;
  auto event = std::make_shared<ReadyEvent>(token, getStatus(scenario, instanceId, process), getData(scenario, instanceId, process));
  schedule(scenario, queue, getProcessReadyTime(scenario, instanceId), token->owner->systemState->getTime(), std::move(event));
}

void StaticDataProvider::readyActivity(Scenario& scenario, EventQueue& queue, const Token* token, BPMNOS::number earliest) const {
  auto instanceId = (size_t)token->owner->root->instance.value();
  auto status = getActivityReadyStatus(scenario, token, earliest);
  auto dueTime = getActivityReadyTime(scenario, instanceId, token->node, status);
  auto event = std::make_shared<ReadyEvent>(token, std::move(status), getData(scenario, instanceId, token->node));
  schedule(scenario, queue, dueTime, token->owner->systemState->getTime(), std::move(event));
}

void StaticDataProvider::completeTask(Scenario& scenario, EventQueue& queue, const Token* token, BPMNOS::number earliest) const {
  auto status = getCompletionStatus(scenario, token, earliest);
  auto dueTime = status[BPMNOS::Model::ExtensionElements::Index::Timestamp].value();
  schedule(scenario, queue, dueTime, token->owner->systemState->getTime(), std::make_shared<CompletionEvent>(token, std::move(status)));
}

void StaticDataProvider::schedule(Scenario& scenario, EventQueue& queue, BPMNOS::number dueTime, BPMNOS::number currentTime, std::shared_ptr<Event> event) {
  if ( dueTime <= currentTime ) {
    queue.push_back(std::move(event));
  }
  else {
    scenario.scheduledEvents.emplace(dueTime, std::move(event));
  }
}

std::optional<BPMNOS::number> StaticDataProvider::getValue(const Scenario& scenario, size_t instanceId, const BPMNOS::Model::Attribute* attribute) const {
  if ( attribute->expression && attribute->expression->type == BPMNOS::Model::Expression::Type::ASSIGN ) {
    // the value is computed from values of the instance which must not change during the run
    std::vector<double> variableValues;
    for ( auto input : attribute->expression->variables ) {
      auto value = input->isImmutable ? getValue(scenario, instanceId, input) : std::nullopt;
      if ( !value.has_value() ) {
        return std::nullopt;
      }
      variableValues.push_back((double)value.value());
    }
    std::vector<std::vector<double>> collectionValues;
    for ( auto input : attribute->expression->collections ) {
      auto collection = input->isImmutable ? getValue(scenario, instanceId, input) : std::nullopt;
      if ( !collection.has_value() ) {
        return std::nullopt;
      }
      collectionValues.push_back(collectionRegistry[(size_t)collection.value()]);
    }
    return BPMNOS::number(attribute->expression->compiled.evaluate(variableValues, collectionValues));
  }
  auto& values = getInstanceValues(scenario, instanceId);
  if ( auto it = values.find(attribute); it != values.end() ) {
    return it->second;
  }
  return std::nullopt;
}

BPMNOS::Values StaticDataProvider::getStatus(const Scenario& scenario, size_t instanceId, const BPMN::Node* node) const {
  BPMNOS::Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->attributes ) {
    result.push_back(getValue(scenario, instanceId, attribute.get()));
  }
  return result;
}

BPMNOS::Values StaticDataProvider::getData(const Scenario& scenario, size_t instanceId, const BPMN::Node* node) const {
  BPMNOS::Values result;
  for ( auto& attribute : node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->data ) {
    result.push_back(getValue(scenario, instanceId, attribute.get()));
  }
  return result;
}
