#include "StochasticDataProvider.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/StateMachine.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/utility/src/InputEncoder.h"
#include "model/utility/src/StringRegistry.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <functional>
#include <stdexcept>

using namespace BPMNOS::Execution;

namespace {

BPMNOS::number convert(double value, BPMNOS::ValueType type) {
  switch ( type ) {
    case BPMNOS::ValueType::INTEGER:
      return BPMNOS::number((int)value);
    case BPMNOS::ValueType::BOOLEAN:
      return BPMNOS::number(value != 0 ? 1 : 0);
    default:
      return BPMNOS::number(value);
  }
}

/// @brief Returns true if an expression assigns the timestamp.
bool assignsTimestamp(const BPMNOS::Model::Expression& expression) {
  auto target = expression.target.value();
  return target->category == BPMNOS::Model::Attribute::Category::STATUS && !target->isObject() && target->index == BPMNOS::Model::ExtensionElements::Index::Timestamp;
}

/// @brief Returns true if a list of attributes declared for a node holds the given attribute.
bool holds(const std::vector<std::unique_ptr<BPMNOS::Model::Attribute>>& attributes, const BPMNOS::Model::Attribute* attribute) {
  return std::ranges::any_of(attributes, [attribute](auto& declared) { return declared.get() == attribute; });
}

} // namespace

StochasticDataProvider::Scenario::Scenario(std::shared_ptr<const StochasticDataProvider> dataProvider, unsigned int seed)
  : StaticDataProvider::Scenario(std::move(dataProvider))
  , seed(seed)
{
}

std::mt19937& StochasticDataProvider::Scenario::getRandomNumberGenerator(size_t instanceId, const BPMN::Node* node) {
  auto [it, inserted] = randomNumberGenerators.try_emplace({instanceId, node});
  if ( inserted ) {
    // the instance is identified by its name rather than by its index in the string registry, which depends
    // on what was registered before, so that the realisation depends on nothing but the seed and the data
    auto instanceHash = std::hash<std::string>{}(stringRegistry[instanceId]);
    auto nodeHash = std::hash<std::string>{}(node->id);
    it->second.seed( static_cast<std::mt19937::result_type>( static_cast<size_t>(seed) ^ (instanceHash * 31) ^ (nodeHash * 17) ) );
  }
  return it->second;
}

StochasticDataProvider::StochasticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, unsigned int seed, std::chrono::milliseconds clockTickDuration)
  : DynamicDataProvider(std::move(model), clockTickDuration)
  , seed(seed)
{
  for ( auto& lookupTable : this->model->lookupTables ) {
    auto table = lookupTable.get();
    handle.addFunction(table->name, [table](const std::vector<double>& args) {
      return table->at(args);
    });
  }
  randomDistributionFactory.registerFunctions(handle);

  // the global values are sampled once, with the seed of the data provider
  std::mt19937 randomNumberGenerator(seed);
  randomDistributionFactory.setCurrentRng(&randomNumberGenerator);
  readInstances(instanceFileOrString, { "INSTANCE_ID", "NODE_ID", "INITIALIZATION", "DISCLOSURE", "READY", "COMPLETION" }, handle);
  randomDistributionFactory.setCurrentRng(nullptr);
}

std::unique_ptr<BPMNOS::Model::Expression> StochasticDataProvider::compileReadyExpression(const InstanceDataReader::Row& row, const std::string& expression) const {
  auto extensionElements = row.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  auto compiled = std::make_unique<BPMNOS::Model::Expression>(handle, BPMNOS::InputEncoder::fragment(expression), extensionElements->attributeRegistry);
  if ( !compiled->target.has_value() ) {
    throw std::runtime_error("StochasticDataProvider: the ready expression of '" + row.node->id + "' must assign an attribute");
  }
  auto target = compiled->target.value();
  // a ready event gives the whole status and the data the scope owns, never inherited data; an object, an element
  // or a field may be assigned as a scalar may
  if ( target->category != BPMNOS::Model::Attribute::Category::STATUS && !holds(extensionElements->data, target) && !holds(extensionElements->dataObjects, target) ) {
    throw std::runtime_error("StochasticDataProvider: the ready expression of '" + row.node->id + "' must assign a status attribute or a data attribute the scope owns");
  }
  if ( assignsTimestamp(*compiled) ) {
    // the timestamp determines when the token becomes ready, so it is sampled when the token arrives, before any
    // other ready expression and before the attributes of the scope exist
    if ( getReadyExpressions(row.instanceId, row.node) ) {
      throw std::runtime_error("StochasticDataProvider: the ready expression of '" + row.node->id + "' assigning the timestamp must be the first ready expression of the node");
    }
    for ( auto input : compiled->inputs ) {
      if ( holds(extensionElements->attributes, input) || holds(extensionElements->statusObjects, input) || holds(extensionElements->data, input) || holds(extensionElements->dataObjects, input) ) {
        throw std::runtime_error("StochasticDataProvider: the ready expression of '" + row.node->id + "' assigning the timestamp must read only what the scope inherits, not '" + input->name + "'");
      }
    }
  }
  return compiled;
}

std::unique_ptr<BPMNOS::Model::Expression> StochasticDataProvider::compileCompletionExpression(const InstanceDataReader::Row& row, const std::string& expression) const {
  auto extensionElements = row.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  auto compiled = std::make_unique<BPMNOS::Model::Expression>(handle, BPMNOS::InputEncoder::fragment(expression), extensionElements->attributeRegistry);
  // a status object, an element or a field of one may be assigned as a scalar may
  if ( !compiled->target.has_value() || compiled->target.value()->category != BPMNOS::Model::Attribute::Category::STATUS ) {
    throw std::runtime_error("StochasticDataProvider: the completion expression of '" + row.node->id + "' must assign a status attribute");
  }
  return compiled;
}

const std::vector<std::unique_ptr<BPMNOS::Model::Expression>>* StochasticDataProvider::getReadyExpressions(size_t instanceId, const BPMN::Node* node) const {
  auto instanceExpressions = readyExpressions.find(instanceId);
  if ( instanceExpressions == readyExpressions.end() ) {
    return nullptr;
  }
  auto nodeExpressions = instanceExpressions->second.find(node);
  if ( nodeExpressions == instanceExpressions->second.end() ) {
    return nullptr;
  }
  return &nodeExpressions->second;
}

bool StochasticDataProvider::initialisedBefore(size_t instanceId, const BPMNOS::Model::Attribute* attribute) const {
  return std::ranges::any_of(initializations, [instanceId, attribute](auto& initialization) {
    return initialization.instanceId == instanceId && initialization.attribute == attribute;
  });
}

void StochasticDataProvider::requireInitialised(const InstanceDataReader& reader, size_t instanceId, const BPMNOS::Model::Expression& expression, const std::string& text) const {
  // a global value is given by the instance data before any instance is read, an object possibly by the model
  for ( auto attribute : expression.variables ) {
    bool known = attribute->isGlobal ? reader.globals.contains(attribute) : initialisedBefore(instanceId, attribute);
    if ( !known ) {
      throw std::runtime_error("StochasticDataProvider: expression '" + text + "' refers to attribute '" + attribute->name + "' without a value");
    }
  }
  for ( auto object : expression.paths ) {
    // a path to an input has no attribute, an input holding its object from the start
    if ( !object ) {
      continue;
    }
    bool known = object->initialObject.has_value() || ( object->isGlobal ? reader.globalObjects.contains(object) : initialisedBefore(instanceId, object) );
    if ( !known ) {
      throw std::runtime_error("StochasticDataProvider: expression '" + text + "' refers to object '" + object->name + "' without a value");
    }
  }
}

void StochasticDataProvider::readValue(InstanceDataReader& reader, const InstanceDataReader::Row& row, [[maybe_unused]] const LIMEX::Handle<double>& rowHandle) {
  enum { DISCLOSURE, READY, COMPLETION };
  auto& disclosure = row.cells[DISCLOSURE];
  auto& ready = row.cells[READY];
  auto& completion = row.cells[COMPLETION];

  if ( !ready.empty() ) {
    // the engine creates event subprocesses and compensation activities itself
    auto activity = row.node->represents<BPMN::Activity>();
    if ( !activity || row.node->represents<BPMN::EventSubProcess>() || activity->isForCompensation ) {
      throw std::runtime_error("StochasticDataProvider: no ready expression may be given for '" + row.node->id + "'");
    }
    auto compiled = compileReadyExpression(row, ready);
    readyExpressions[row.instanceId][row.node].push_back(std::move(compiled));
  }

  if ( !completion.empty() ) {
    // send, receive and decision tasks complete as the model determines
    if (
      !row.node->represents<BPMN::Task>() ||
      row.node->represents<BPMN::SendTask>() ||
      row.node->represents<BPMN::ReceiveTask>() ||
      row.node->represents<BPMNOS::Model::DecisionTask>()
    ) {
      throw std::runtime_error("StochasticDataProvider: no completion expression may be given for '" + row.node->id + "'");
    }
    completionExpressions[row.instanceId][row.node].push_back(compileCompletionExpression(row, completion));
  }

  if ( row.initialization.empty() ) {
    if ( !disclosure.empty() ) {
      throw std::runtime_error("StochasticDataProvider: disclosure of '" + row.node->id + "' requires an initialization in the same row");
    }
    return;
  }

  auto& attributeRegistry = row.node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry;
  auto disclosureText = disclosure.empty() ? std::string("0") : disclosure;
  auto disclosureExpression = std::make_unique<BPMNOS::Model::Expression>(handle, BPMNOS::InputEncoder::fragment(disclosureText), attributeRegistry);
  if ( reader.assignsObject(row) ) {
    auto [object, initializer] = reader.lookupObject(row);
    std::vector<std::unique_ptr<BPMNOS::Model::Expression>> members;
    for ( auto& member : initializer.members() ) {
      members.push_back( std::make_unique<BPMNOS::Model::Expression>(handle, BPMNOS::InputEncoder::fragment(member.expression), attributeRegistry) );
      requireInitialised(reader, row.instanceId, *members.back(), member.expression);
    }
    initializations.push_back({
      row.instanceId,
      row.node,
      object,
      nullptr,
      ObjectInitialization{ std::move(initializer), std::move(members), reader.getSchema(object) },
      std::move(disclosureExpression)
    });
  }
  else {
    auto [attribute, expression] = reader.lookupAttribute(row.node, row.initialization);
    auto value = std::make_unique<BPMNOS::Model::Expression>(handle, BPMNOS::InputEncoder::fragment(expression), attributeRegistry);
    requireInitialised(reader, row.instanceId, *value, expression);
    initializations.push_back({
      row.instanceId,
      row.node,
      attribute,
      std::move(value),
      std::nullopt,
      std::move(disclosureExpression)
    });
  }

  // the disclosure may read the attribute or object the row initialises
  requireInitialised(reader, row.instanceId, *initializations.back().disclosure, disclosureText);
}

std::unique_ptr<BPMNOS::Execution::Scenario> StochasticDataProvider::createScenario(unsigned int realisation) const {
  auto scenario = std::make_unique<Scenario>(std::static_pointer_cast<const StochasticDataProvider>(shared_from_this()), seed + realisation);
  sample(*scenario, nullptr, std::numeric_limits<BPMNOS::number>::lowest());
  return scenario;
}

std::unique_ptr<BPMNOS::Execution::Scenario> StochasticDataProvider::forkScenario(const Execution::Scenario& scenario, unsigned int index) const {
  auto& original = static_cast<const Scenario&>(scenario);
  auto fork = std::make_unique<Scenario>(std::static_pointer_cast<const StochasticDataProvider>(shared_from_this()), original.seed + index + 1);
  // the fork agrees with the run before the instant following its current time
  sample(*fork, &original, original.time + 1);
  return fork;
}

void StochasticDataProvider::sample(Scenario& scenario, const Scenario* original, BPMNOS::number spawnTime) const {
  for ( auto& instance : instances ) {
    scenario.values[instance.id] = instance.values;
    scenario.objects[instance.id] = instance.objects;
    scenario.instantiationTimes[instance.id] = instance.instantiationTime;
    scenario.disclosureTimes[instance.id][instance.process] = 0;
  }

  // the initializations are sampled in the order of the rows, so that each may refer to those before it
  for ( size_t k = 0; k < initializations.size(); k++ ) {
    auto& initialization = initializations[k];
    auto& values = scenario.values.at(initialization.instanceId);
    auto& objects = scenario.objects.at(initialization.instanceId);
    auto process = getInstance(initialization.instanceId).process;
    bool isTimestamp = ( !initialization.object.has_value() && initialization.attribute == process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].get() );

    BPMNOS::number value = 0;
    std::shared_ptr<const BPMNOS::Object> object;
    BPMNOS::number disclosureTime;
    if ( original && original->initializationDisclosureTimes[k] < spawnTime ) {
      // the initialization was disclosed before the spawn time, so the fork keeps it
      if ( initialization.object.has_value() ) {
        object = original->objects.at(initialization.instanceId).at(initialization.attribute);
      }
      else {
        value = original->values.at(initialization.instanceId).at(initialization.attribute);
      }
      disclosureTime = original->initializationDisclosureTimes[k];
    }
    else {
      auto& attributeRegistry = initialization.node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry;
      BPMNOS::Status status(attributeRegistry.statusAttributes.size());
      BPMNOS::Data data(attributeRegistry.dataAttributes.size());
      // the global attributes are the first data attributes
      std::copy(globals.attributes.begin(), globals.attributes.end(), data.attributes.begin());
      for ( auto& [attribute, attributeValue] : values ) {
        if ( attribute->category == BPMNOS::Model::Attribute::Category::STATUS && attribute->index < status.attributes.size() && attributeRegistry.contains(attribute) ) {
          status.attributes[attribute->index] = attributeValue;
        }
        else if ( attribute->category == BPMNOS::Model::Attribute::Category::DATA && attribute->index < data.attributes.size() && attributeRegistry.contains(attribute) ) {
          data.attributes[attribute->index] = attributeValue;
        }
      }
      // the objects given by earlier rows, every other object holding its default, the global objects being the
      // first data objects
      auto objectOf = [&](const BPMNOS::Model::Attribute* attribute) {
        auto it = objects.find(attribute);
        return it != objects.end() ? it->second : getDefaultObject(attribute);
      };
      status.objects.resize(attributeRegistry.statusObjects.size());
      for ( auto attribute : attributeRegistry.statusObjects ) {
        status.objects[attribute->index] = objectOf(attribute);
      }
      data.objects.resize(attributeRegistry.dataObjects.size());
      for ( auto attribute : attributeRegistry.dataObjects ) {
        data.objects[attribute->index] = ( attribute->index < globals.objects.size() ) ? globals.objects[attribute->index] : objectOf(attribute);
      }

      // an expression of the row that yields no value is refused, as when the instance data are read
      auto valueOf = [&](const BPMNOS::Model::Expression& expression) {
        auto result = expression.execute(status, data);
        if ( !result.has_value() ) {
          throw std::runtime_error("StochasticDataProvider: the initialization of '" + initialization.attribute->name + "' of instance '" + stringRegistry[initialization.instanceId] + "' or its disclosure yields no value");
        }
        return result.value();
      };

      randomDistributionFactory.setCurrentRng(&scenario.getRandomNumberGenerator(initialization.instanceId, initialization.node));
      if ( initialization.object.has_value() ) {
        // the members read the values before the row, the object being given only when all its members are evaluated
        std::vector<double> memberValues;
        for ( auto& member : initialization.object->members ) {
          memberValues.push_back( valueOf(*member) );
        }
        object = InstanceDataReader::instantiate(initialization.object->schema, initialization.attribute, initialization.object->initializer, memberValues);
        attributeRegistry.setObject(initialization.attribute, status, data, object);
      }
      else {
        auto evaluate = [&]() {
          return convert(valueOf(*initialization.value), initialization.attribute->type);
        };
        value = evaluate();
        if ( isTimestamp ) {
          // a fork samples a timestamp before the spawn time again, and sets it to the spawn time at last
          for ( int tries = 1; value < spawnTime && tries < maxResamplingTries; tries++ ) {
            value = evaluate();
          }
          value = std::max(value, spawnTime);
        }
        if ( initialization.attribute->category == BPMNOS::Model::Attribute::Category::STATUS ) {
          status.attributes[initialization.attribute->index] = value;
        }
        else if ( initialization.attribute->category == BPMNOS::Model::Attribute::Category::DATA ) {
          data.attributes[initialization.attribute->index] = value;
        }
      }
      auto evaluateDisclosure = [&]() {
        return BPMNOS::number(std::ceil(valueOf(*initialization.disclosure)));
      };
      disclosureTime = evaluateDisclosure();
      // a fork samples a disclosure time before the spawn time again, and sets it to the spawn time at last
      for ( int tries = 1; disclosureTime < spawnTime && tries < maxResamplingTries; tries++ ) {
        disclosureTime = evaluateDisclosure();
      }
      disclosureTime = std::max(disclosureTime, spawnTime);
      randomDistributionFactory.setCurrentRng(nullptr);
    }

    if ( initialization.object.has_value() ) {
      objects[initialization.attribute] = std::move(object);
    }
    else {
      values[initialization.attribute] = value;
    }
    scenario.initializationDisclosureTimes.push_back(disclosureTime);
    if ( isTimestamp ) {
      // instances are instantiated at integral times
      scenario.instantiationTimes[initialization.instanceId] = BPMNOS::number(std::ceil((double)value));
    }

    // the disclosure time of a node is at least that of the scope containing it
    auto& nodeDisclosureTimes = scenario.disclosureTimes.at(initialization.instanceId);
    if ( auto childNode = initialization.node->represents<BPMN::ChildNode>() ) {
      if ( auto scope = nodeDisclosureTimes.find(childNode->parent); scope != nodeDisclosureTimes.end() ) {
        disclosureTime = std::max(disclosureTime, scope->second);
      }
    }
    auto [it, inserted] = nodeDisclosureTimes.try_emplace(initialization.node, disclosureTime);
    if ( !inserted ) {
      it->second = std::max(it->second, disclosureTime);
    }
  }
}

const std::unordered_map<const BPMNOS::Model::Attribute*, BPMNOS::number>& StochasticDataProvider::getInstanceValues(const StaticDataProvider::Scenario& scenario, size_t instanceId) const {
  return static_cast<const Scenario&>(scenario).values.at(instanceId);
}

const std::unordered_map<const BPMNOS::Model::Attribute*, std::shared_ptr<const BPMNOS::Object>>& StochasticDataProvider::getInstanceObjects(const StaticDataProvider::Scenario& scenario, size_t instanceId) const {
  return static_cast<const Scenario&>(scenario).objects.at(instanceId);
}

const std::unordered_map<const BPMN::Node*, BPMNOS::number>& StochasticDataProvider::getDisclosureTimes(const StaticDataProvider::Scenario& scenario, size_t instanceId) const {
  return static_cast<const Scenario&>(scenario).disclosureTimes.at(instanceId);
}

BPMNOS::number StochasticDataProvider::getProcessReadyTime(const StaticDataProvider::Scenario& scenario, size_t instanceId) const {
  return std::max(static_cast<const Scenario&>(scenario).instantiationTimes.at(instanceId), getKnownTime(scenario, instanceId));
}

std::optional<BPMNOS::number> StochasticDataProvider::getReadyTimestamp(StaticDataProvider::Scenario& scenario, const Token* token, BPMNOS::number earliest) const {
  auto instanceId = (size_t)token->owner->root->instance.value();
  auto expressions = getReadyExpressions(instanceId, token->node);
  if ( !expressions || !assignsTimestamp(*expressions->front()) ) {
    return std::nullopt;
  }
  auto& expression = *expressions->front();
  auto target = expression.target.value();
  randomDistributionFactory.setCurrentRng(&static_cast<Scenario&>(scenario).getRandomNumberGenerator(instanceId, token->node));
  auto evaluate = [&]() -> std::optional<BPMNOS::number> {
    if ( auto value = expression.execute(token->status, *token->data) ) {
      return convert(value.value(), target->type);
    }
    return std::nullopt;
  };
  auto timestamp = evaluate();
  // a timestamp sampled for a token awaiting its ready event in an installed system state is sampled again while
  // it precedes the time of that state, and is then set to that time, since the event has not happened before
  for ( int tries = 1; timestamp.has_value() && timestamp.value() < earliest && tries < maxResamplingTries; tries++ ) {
    timestamp = evaluate();
  }
  if ( timestamp.has_value() && timestamp.value() < earliest ) {
    timestamp = earliest;
  }
  randomDistributionFactory.setCurrentRng(nullptr);
  return timestamp;
}

BPMNOS::number StochasticDataProvider::getActivityReadyTime(const StaticDataProvider::Scenario& scenario, size_t instanceId, const BPMN::Node* activity, std::optional<BPMNOS::number> readyTimestamp) const {
  // the token becomes ready once the timestamp a ready value gives is reached
  auto readyTime = DynamicDataProvider::getActivityReadyTime(scenario, instanceId, activity, readyTimestamp);
  return readyTimestamp.has_value() ? std::max(readyTime, readyTimestamp.value()) : readyTime;
}

void StochasticDataProvider::applyReadyValues(StaticDataProvider::Scenario& scenario, const Token* token, InstanceValues& values) const {
  auto instanceId = (size_t)token->owner->root->instance.value();
  auto expressions = getReadyExpressions(instanceId, token->node);
  if ( !expressions ) {
    return;
  }
  randomDistributionFactory.setCurrentRng(&static_cast<Scenario&>(scenario).getRandomNumberGenerator(instanceId, token->node));
  for ( auto& expression : *expressions ) {
    // the timestamp was sampled when the token arrived
    if ( !assignsTimestamp(*expression) ) {
      values.assign(*expression);
    }
  }
  randomDistributionFactory.setCurrentRng(nullptr);
}

BPMNOS::Status StochasticDataProvider::getCompletionStatus(StaticDataProvider::Scenario& scenario, const Token* token, BPMNOS::number earliest) const {
  auto status = DynamicDataProvider::getCompletionStatus(scenario, token, earliest);
  computeStatus(static_cast<Scenario&>(scenario), (size_t)token->owner->root->instance.value(), token->node, status, *token->data, earliest);
  return status;
}

void StochasticDataProvider::computeStatus(Scenario& scenario, size_t instanceId, const BPMN::Node* node, BPMNOS::Status& status, BPMNOS::SharedData& data, BPMNOS::number earliest) const {
  auto instanceExpressions = completionExpressions.find(instanceId);
  if ( instanceExpressions == completionExpressions.end() ) {
    return;
  }
  auto nodeExpressions = instanceExpressions->second.find(node);
  if ( nodeExpressions == instanceExpressions->second.end() ) {
    return;
  }
  randomDistributionFactory.setCurrentRng(&scenario.getRandomNumberGenerator(instanceId, node));
  const auto initialStatus = status;
  auto evaluate = [&]() {
    status = initialStatus;
    for ( auto& expression : nodeExpressions->second ) {
      if ( expression->writesToObject() ) {
        // the status object is copied on write, so that the status restored before a new sample keeps the object
        // it was computed from
        expression->writeToObject(status, data);
      }
      else if ( auto value = expression->execute(status, data) ) {
        auto target = expression->target.value();
        status.attributes[target->index] = convert(value.value(), target->type);
      }
    }
  };
  evaluate();
  // a status computed anew for a token awaiting its event in an installed system state is sampled again
  // while its timestamp precedes the time of that state, and its timestamp is then set to that time, since
  // the event has not happened before; a status computed otherwise has the lowest number as earliest time
  constexpr auto Timestamp = BPMNOS::Model::ExtensionElements::Index::Timestamp;
  auto tooEarly = [&]() {
    return status.attributes[Timestamp].has_value() && status.attributes[Timestamp].value() < earliest;
  };
  // sampling again can change the timestamp only if an expression assigns it
  bool timestampSampled = std::ranges::any_of(nodeExpressions->second, [](auto& expression) {
    auto target = expression->target.value();
    return target->category == BPMNOS::Model::Attribute::Category::STATUS && target->index == Timestamp;
  });
  for ( int tries = 1; timestampSampled && tooEarly() && tries < maxResamplingTries; tries++ ) {
    evaluate();
  }
  if ( tooEarly() ) {
    status.attributes[Timestamp] = earliest;
  }
  randomDistributionFactory.setCurrentRng(nullptr);
}
