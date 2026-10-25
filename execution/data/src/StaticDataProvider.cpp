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
#include "model/utility/src/ObjectRegistry.h"
#include "model/utility/src/StringRegistry.h"
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
  // the sizes are declared for every instance, so they are known before any value is read
  for ( auto& row : reader.rows ) {
    if ( row.declaresSizes ) {
      reader.declareSizes(row);
    }
  }
  for ( auto& row : reader.rows ) {
    if ( row.declaresSizes ) {
      continue;
    }
    if ( !row.node ) {
      // a global value is given once, for every run
      if ( reader.assignsObject(row) ) {
        reader.setObject(row, handle);
      }
      else {
        reader.evaluateGlobal(row.initialization, handle);
      }
    }
    else {
      readValue(reader, row, handle);
    }
  }

  // the global attributes are given their values in declaration order, each the value of its initial
  // expression if the model assigns it, otherwise the value given, so that an expression reads the global
  // attributes declared before it
  globals = BPMNOS::Data(this->model->attributes.size());
  for ( auto& attribute : this->model->attributes ) {
    if ( auto expression = std::get_if< std::unique_ptr<const BPMNOS::Model::Expression> >(&attribute->initialization) ) {
      // the global attributes are the first data attributes
      auto value = (*expression)->execute(BPMNOS::Status{}, globals);
      if ( !value.has_value() ) {
        throw std::runtime_error("StaticDataProvider: failed to evaluate global attribute '" + attribute->id + "'");
      }
      globals.attributes[attribute->index] = value.value();
    }
    else if ( auto it = reader.globals.find(attribute.get()); it != reader.globals.end() ) {
      globals.attributes[attribute->index] = it->second;
    }
  }

  // the objects: the default of each, from the model's initial value or with undefined values in the sizes
  // declared, an open dimension having no elements, and the global objects
  auto convert = [&reader](const BPMNOS::Model::Attribute* object, size_t index, const std::string& owner) -> std::shared_ptr<const BPMNOS::Object> {
    try {
      auto schema = reader.getSchema(object);
      auto& literalSchema = objectRegistry.schema(index);
      return schema.supports(literalSchema) ? objectRegistry[index] : std::make_shared<const BPMNOS::Object>(schema, *objectRegistry[index], literalSchema);
    }
    catch ( const std::exception& error ) {
      throw std::runtime_error("StaticDataProvider: illegal value of object '" + object->id + "' " + owner + ".\n" + error.what());
    }
  };
  for ( auto object : objectsOf() ) {
    auto schema = reader.getSchema(object);
    if ( auto literal = std::get_if<size_t>(&object->initialization) ) {
      defaultObjects[object] = convert(object, *literal, "in the model");
    }
    else {
      // an open dimension has no elements until an assignment gives it its length
      defaultObjects[object] = schema.undefinedObject();
    }
  }
  // the default content with the sizes declared at the node throwing it or, for a signal, by its name, every node of
  // a signal sharing one
  for ( auto& [name, schema] : reader.signalSchemas ) {
    defaultContents[name] = schema.undefinedObject();
  }
  // the effective schemas are kept, the content of every node of a signal having the schema declared by its name
  schemas = reader.schemas;
  for ( auto& process : this->model->processes ) {
    for ( auto node : process->find_all([](const BPMN::Node* node) { return node->extensionElements && node->extensionElements->represents<const BPMNOS::Model::ExtensionElements>() && node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->content; }) ) {
      auto extensionElements = node->extensionElements->as<const BPMNOS::Model::ExtensionElements>();
      auto content = extensionElements->content.get();
      if ( auto signalDefinition = extensionElements->signalDefinition.get() ) {
        defaultObjects[content] = defaultContents.at(signalDefinition->name);
        if ( auto it = reader.signalSchemas.find(signalDefinition->name); it != reader.signalSchemas.end() ) {
          schemas[content] = it->second;
        }
      }
      else if ( reader.schemas.contains(content) ) {
        defaultObjects[content] = reader.getSchema(content).undefinedObject();
      }
    }
  }
  for ( auto& object : this->model->objects ) {
    if ( auto it = reader.globalObjects.find(object.get()); it != reader.globalObjects.end() ) {
      globals.objects.push_back( it->second );
    }
    else {
      globals.objects.push_back( getDefaultObject(object.get()) );
    }
  }

  // the instances are kept in the order of their first rows
  for ( auto& row : reader.rows ) {
    if ( !row.node || row.declaresSizes || instancePositions.contains(row.instanceId) ) {
      continue;
    }
    auto instanceId = row.instanceId;
    auto process = reader.processes.at(instanceId);
    auto values = reader.values.contains(instanceId) ? reader.values.at(instanceId) : std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>{};
    reader.addDefaultValues(instanceId, values);
    auto timestampAttribute = process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].get();
    // instances are instantiated at integral times
    auto instantiationTime = BPMNOS::number(std::ceil((double)values.at(timestampAttribute)));
    // the objects the instance data gives, every other object of the instance taking its default
    auto objects = reader.objects.contains(instanceId) ? reader.objects.at(instanceId) : std::unordered_map<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>>{};
    instancePositions[instanceId] = instances.size();
    instances.push_back(Instance{instanceId, process, std::move(values), instantiationTime, std::move(objects)});
  }
}

void StaticDataProvider::readValue(InstanceDataReader& reader, const InstanceDataReader::Row& row, const LIMEX::Handle<double>& handle) {
  if ( !row.initialization.empty() && reader.assignsObject(row) ) {
    reader.setObject(row, handle);
  }
  else if ( !row.initialization.empty() ) {
    auto [attribute, expression] = reader.lookupAttribute(row.node, row.initialization);
    reader.setValue(row.instanceId, attribute, reader.evaluate(row.instanceId, row.node, expression, std::get<BPMNOS::ValueType>(attribute->type), handle));
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

BPMNOS::Data StaticDataProvider::getGlobals([[maybe_unused]] const Execution::Scenario& scenario) const {
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
        auto event = instantiate(scenario, instance);
        schedule(scenario, queue, getKnownTime(scenario, instance.id), systemState->getTime(), [event](Scenario&) { return event; });
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
        if ( auto event = it->second(scenario) ) {
          queue.push_back(std::move(event));
        }
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
    ( systemState->stateMachine->tokens.empty() && scenario.scheduledEvents.empty() )
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

const std::unordered_map<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>>& StaticDataProvider::getInstanceObjects([[maybe_unused]] const Scenario& scenario, size_t instanceId) const {
  return getInstance(instanceId).objects;
}

BPMNOS::number StaticDataProvider::getKnownTime([[maybe_unused]] const Scenario& scenario, [[maybe_unused]] size_t instanceId) const {
  return std::numeric_limits<BPMNOS::number>::lowest();
}

BPMNOS::number StaticDataProvider::getProcessReadyTime([[maybe_unused]] const Scenario& scenario, size_t instanceId) const {
  return getInstance(instanceId).instantiationTime;
}

std::optional<BPMNOS::number> StaticDataProvider::getReadyTimestamp([[maybe_unused]] Scenario& scenario, [[maybe_unused]] const Token* token, [[maybe_unused]] BPMNOS::number earliest) const {
  return std::nullopt;
}

BPMNOS::number StaticDataProvider::getActivityReadyTime([[maybe_unused]] const Scenario& scenario, [[maybe_unused]] size_t instanceId, [[maybe_unused]] const BPMN::Node* activity, [[maybe_unused]] std::optional<BPMNOS::number> readyTimestamp) const {
  return std::numeric_limits<BPMNOS::number>::lowest();
}

void StaticDataProvider::applyReadyValues([[maybe_unused]] Scenario& scenario, [[maybe_unused]] const Token* token, [[maybe_unused]] InstanceValues& values) const {
}

BPMNOS::Status StaticDataProvider::getCompletionStatus([[maybe_unused]] Scenario& scenario, const Token* token, [[maybe_unused]] BPMNOS::number earliest) const {
  return token->status;
}

std::shared_ptr<Event> StaticDataProvider::instantiate(const Scenario& scenario, const Instance& instance) const {
  // the instantiation gives the instance its identity and timestamp, its other values being given when its
  // process becomes ready; the objects are laid out with their defaults, so that every object of the instance has
  // its slot
  auto extensionElements = instance.process->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  BPMNOS::Status status( extensionElements->attributes.size() );
  auto timestampAttribute = extensionElements->attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].get();
  status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp] = getInstanceValues(scenario, instance.id).at(timestampAttribute);
  status.objects.reserve(extensionElements->statusObjects.size());
  for ( auto& object : extensionElements->statusObjects ) {
    status.objects.push_back( getDefaultObject(object.get()) );
  }
  BPMNOS::Data data( extensionElements->data.size() );
  data.attributes[BPMNOS::Model::ExtensionElements::Position::Instance] = BPMNOS::number(instance.id);
  data.objects.reserve(extensionElements->dataObjects.size());
  for ( auto& object : extensionElements->dataObjects ) {
    data.objects.push_back( getDefaultObject(object.get()) );
  }
  return std::make_shared<InstantiationEvent>(instance.process, std::move(status), std::move(data));
}

void StaticDataProvider::readyProcess(Scenario& scenario, EventQueue& queue, const Token* token) const {
  auto instanceId = (size_t)token->getInstanceId();
  // the event is built when it becomes due, so that the initial values read the globals of that time
  std::weak_ptr<const Token> token_ptr = token->weak_from_this();
  schedule(scenario, queue, getProcessReadyTime(scenario, instanceId), token->owner->systemState->getTime(), [this, token_ptr, instanceId](Scenario& dueScenario) -> std::shared_ptr<Event> {
    auto process = token_ptr.lock();
    if ( !process ) {
      return nullptr;
    }
    InstanceValues values(*this, process->node, dueScenario.time, BPMNOS::Status{}, process->owner->data, process->owned->instance, getInstanceValues(dueScenario, instanceId), getInstanceObjects(dueScenario, instanceId));
    return std::make_shared<ReadyEvent>(process.get(), std::move(values.status), std::move(values.data));
  });
}

void StaticDataProvider::readyActivity(Scenario& scenario, EventQueue& queue, const Token* token, BPMNOS::number earliest) const {
  auto instanceId = (size_t)token->owner->root->instance.value();
  // the timestamp a ready value gives is determined when the token arrives, since the due time depends on it
  auto dueTime = getActivityReadyTime(scenario, instanceId, token->node, getReadyTimestamp(scenario, token, earliest));
  // the event is built when it becomes due, so that the initial values read the data and globals of that time
  std::weak_ptr<const Token> token_ptr = token->weak_from_this();
  schedule(scenario, queue, dueTime, token->owner->systemState->getTime(), [this, token_ptr, instanceId](Scenario& dueScenario) -> std::shared_ptr<Event> {
    auto activity = token_ptr.lock();
    if ( !activity ) {
      return nullptr;
    }
    InstanceValues values(*this, activity->node, dueScenario.time, activity->status, *activity->data, std::nullopt, getInstanceValues(dueScenario, instanceId), getInstanceObjects(dueScenario, instanceId));
    applyReadyValues(dueScenario, activity.get(), values);
    return std::make_shared<ReadyEvent>(activity.get(), std::move(values.status), std::move(values.data));
  });
}

void StaticDataProvider::completeTask(Scenario& scenario, EventQueue& queue, const Token* token, BPMNOS::number earliest) const {
  auto status = getCompletionStatus(scenario, token, earliest);
  auto dueTime = status.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].value();
  auto event = std::make_shared<CompletionEvent>(token, std::move(status));
  schedule(scenario, queue, dueTime, token->owner->systemState->getTime(), [event](Scenario&) { return event; });
}

void StaticDataProvider::schedule(Scenario& scenario, EventQueue& queue, BPMNOS::number dueTime, BPMNOS::number currentTime, std::function<std::shared_ptr<Event>(Scenario&)> produce) {
  if ( dueTime <= currentTime ) {
    if ( auto event = produce(scenario) ) {
      queue.push_back(std::move(event));
    }
  }
  else {
    scenario.scheduledEvents.emplace(dueTime, std::move(produce));
  }
}

std::vector<const BPMNOS::Model::Attribute*> StaticDataProvider::objectsOf(const BPMN::Process* process) const {
  std::vector<const BPMNOS::Model::Attribute*> objects;
  auto collect = [&objects](const BPMN::Node* node) {
    // not every node owns extension elements
    if ( !node->extensionElements ) {
      return;
    }
    if ( auto extensionElements = node->extensionElements->represents<const BPMNOS::Model::ExtensionElements>() ) {
      for ( auto list : { &extensionElements->statusObjects, &extensionElements->dataObjects } ) {
        for ( auto& object : *list ) {
          objects.push_back(object.get());
        }
      }
    }
  };
  for ( auto& candidate : this->model->processes ) {
    if ( process && candidate.get() != process ) {
      continue;
    }
    collect(candidate.get());
    for ( auto node : candidate->find_all([](const BPMN::Node*) { return true; }) ) {
      collect(node);
    }
  }
  if ( !process ) {
    for ( auto& object : this->model->objects ) {
      objects.push_back(object.get());
    }
  }
  return objects;
}
