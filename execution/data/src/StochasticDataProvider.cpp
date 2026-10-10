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

std::unique_ptr<BPMNOS::Model::Expression> StochasticDataProvider::compileStatusExpression(const InstanceDataReader::Row& row, const std::string& expression, const std::string& column) const {
  auto extensionElements = row.node->extensionElements->as<BPMNOS::Model::ExtensionElements>();
  auto compiled = std::make_unique<BPMNOS::Model::Expression>(handle, BPMNOS::InputEncoder::fragment(expression), extensionElements->attributeRegistry);
  if ( !compiled->target.has_value() || compiled->target.value()->category != BPMNOS::Model::Attribute::Category::STATUS ) {
    throw std::runtime_error("StochasticDataProvider: the " + column + " expression of '" + row.node->id + "' must assign a status attribute");
  }
  return compiled;
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
    readyExpressions[row.instanceId][row.node].push_back(compileStatusExpression(row, ready, "ready"));
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
    completionExpressions[row.instanceId][row.node].push_back(compileStatusExpression(row, completion, "completion"));
  }

  if ( row.initialization.empty() ) {
    if ( !disclosure.empty() ) {
      throw std::runtime_error("StochasticDataProvider: disclosure of '" + row.node->id + "' requires an initialization in the same row");
    }
    return;
  }

  auto [attribute, expression] = reader.lookupAttribute(row.node, row.initialization);
  auto& attributeRegistry = row.node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry;
  initializations.push_back({
    row.instanceId,
    row.node,
    attribute,
    std::make_unique<BPMNOS::Model::Expression>(handle, BPMNOS::InputEncoder::fragment(expression), attributeRegistry),
    std::make_unique<BPMNOS::Model::Expression>(handle, BPMNOS::InputEncoder::fragment(disclosure.empty() ? "0" : disclosure), attributeRegistry)
  });
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
    scenario.instantiationTimes[instance.id] = instance.instantiationTime;
    scenario.disclosureTimes[instance.id][instance.process] = 0;
  }

  // the initializations are sampled in the order of the rows, so that each may refer to those before it
  for ( size_t k = 0; k < initializations.size(); k++ ) {
    auto& initialization = initializations[k];
    auto& values = scenario.values.at(initialization.instanceId);
    auto process = getInstance(initialization.instanceId).process;
    bool isTimestamp = ( initialization.attribute == process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].get() );

    BPMNOS::number value;
    BPMNOS::number disclosureTime;
    if ( original && original->initializationDisclosureTimes[k] < spawnTime ) {
      // the initialization was disclosed before the spawn time, so the fork keeps it
      value = original->values.at(initialization.instanceId).at(initialization.attribute);
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

      randomDistributionFactory.setCurrentRng(&scenario.getRandomNumberGenerator(initialization.instanceId, initialization.node));
      auto evaluate = [&]() {
        return convert(initialization.value->execute(status, data).value_or(0), initialization.attribute->type);
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
      auto evaluateDisclosure = [&]() {
        return BPMNOS::number(std::ceil(initialization.disclosure->execute(status, data).value_or(0)));
      };
      disclosureTime = evaluateDisclosure();
      // a fork samples a disclosure time before the spawn time again, and sets it to the spawn time at last
      for ( int tries = 1; disclosureTime < spawnTime && tries < maxResamplingTries; tries++ ) {
        disclosureTime = evaluateDisclosure();
      }
      disclosureTime = std::max(disclosureTime, spawnTime);
      randomDistributionFactory.setCurrentRng(nullptr);
    }

    values[initialization.attribute] = value;
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

const std::unordered_map<const BPMN::Node*, BPMNOS::number>& StochasticDataProvider::getDisclosureTimes(const StaticDataProvider::Scenario& scenario, size_t instanceId) const {
  return static_cast<const Scenario&>(scenario).disclosureTimes.at(instanceId);
}

BPMNOS::number StochasticDataProvider::getProcessReadyTime(const StaticDataProvider::Scenario& scenario, size_t instanceId) const {
  return std::max(static_cast<const Scenario&>(scenario).instantiationTimes.at(instanceId), getKnownTime(scenario, instanceId));
}

BPMNOS::Status StochasticDataProvider::getActivityReadyStatus(StaticDataProvider::Scenario& scenario, const Token* token, BPMNOS::number earliest) const {
  auto status = DynamicDataProvider::getActivityReadyStatus(scenario, token, earliest);
  computeStatus(static_cast<Scenario&>(scenario), readyExpressions, (size_t)token->owner->root->instance.value(), token->node, status, *token->data, earliest);
  return status;
}

BPMNOS::number StochasticDataProvider::getActivityReadyTime(const StaticDataProvider::Scenario& scenario, size_t instanceId, const BPMN::Node* activity, const BPMNOS::Status& readyStatus) const {
  // the token becomes ready once the timestamp of the status it becomes ready with is reached
  return std::max(DynamicDataProvider::getActivityReadyTime(scenario, instanceId, activity, readyStatus), readyStatus.attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].value());
}

BPMNOS::Status StochasticDataProvider::getCompletionStatus(StaticDataProvider::Scenario& scenario, const Token* token, BPMNOS::number earliest) const {
  auto status = DynamicDataProvider::getCompletionStatus(scenario, token, earliest);
  computeStatus(static_cast<Scenario&>(scenario), completionExpressions, (size_t)token->owner->root->instance.value(), token->node, status, *token->data, earliest);
  return status;
}

void StochasticDataProvider::computeStatus(Scenario& scenario, const Expressions& expressions, size_t instanceId, const BPMN::Node* node, BPMNOS::Status& status, const BPMNOS::SharedData& data, BPMNOS::number earliest) const {
  auto instanceExpressions = expressions.find(instanceId);
  if ( instanceExpressions == expressions.end() ) {
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
      if ( auto value = expression->execute(status, data) ) {
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
