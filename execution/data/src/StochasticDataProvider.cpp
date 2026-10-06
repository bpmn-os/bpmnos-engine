#include "StochasticDataProvider.h"
#include "execution/engine/src/Token.h"
#include "execution/engine/src/StateMachine.h"
#include "model/bpmnos/src/DecisionTask.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"
#include "model/utility/src/InputEncoder.h"
#include <cmath>
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
    auto nodeHash = std::hash<std::string>{}(node->id);
    it->second.seed( static_cast<std::mt19937::result_type>( static_cast<size_t>(seed) ^ (instanceId * 31) ^ (nodeHash * 17) ) );
  }
  return it->second;
}

StochasticDataProvider::StochasticDataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, const std::string& instanceFileOrString, unsigned int seed, unsigned int clockTickDuration)
  : DynamicDataProvider(std::move(model), clockTickDuration)
  , seed(seed)
{
  for ( auto& lookupTable : sharedModel->lookupTables ) {
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

std::unique_ptr<StochasticDataProvider::Scenario> StochasticDataProvider::createScenario(unsigned int realisation) const {
  auto scenario = std::make_unique<Scenario>(std::static_pointer_cast<const StochasticDataProvider>(shared_from_this()), seed + realisation);
  for ( auto& [instanceId, instance] : instances ) {
    scenario->values[instanceId] = instance.values;
    scenario->instantiationTimes[instanceId] = instance.instantiationTime;
    scenario->disclosureTimes[instanceId][instance.process] = 0;
  }

  // the initializations are sampled in the order of the rows, so that each may refer to those before it
  for ( auto& initialization : initializations ) {
    auto& values = scenario->values.at(initialization.instanceId);
    auto& attributeRegistry = initialization.node->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributeRegistry;
    BPMNOS::Values status(attributeRegistry.statusAttributes.size());
    BPMNOS::Values data(attributeRegistry.dataAttributes.size());
    for ( auto& [attribute, value] : values ) {
      if ( attribute->category == BPMNOS::Model::Attribute::Category::STATUS && attribute->index < status.size() && attributeRegistry.contains(attribute) ) {
        status[attribute->index] = value;
      }
      else if ( attribute->category == BPMNOS::Model::Attribute::Category::DATA && attribute->index < data.size() && attributeRegistry.contains(attribute) ) {
        data[attribute->index] = value;
      }
    }

    randomDistributionFactory.setCurrentRng(&scenario->getRandomNumberGenerator(initialization.instanceId, initialization.node));
    auto value = convert(initialization.value->execute(status, data, globals).value_or(0), initialization.attribute->type);
    values[initialization.attribute] = value;
    if ( initialization.attribute->category == BPMNOS::Model::Attribute::Category::STATUS ) {
      status[initialization.attribute->index] = value;
    }
    else if ( initialization.attribute->category == BPMNOS::Model::Attribute::Category::DATA ) {
      data[initialization.attribute->index] = value;
    }
    auto disclosureTime = BPMNOS::number(std::ceil(initialization.disclosure->execute(status, data, globals).value_or(0)));
    randomDistributionFactory.setCurrentRng(nullptr);

    auto process = instances.at(initialization.instanceId).process;
    if ( initialization.attribute == process->extensionElements->as<BPMNOS::Model::ExtensionElements>()->attributes[BPMNOS::Model::ExtensionElements::Index::Timestamp].get() ) {
      // instances are instantiated at integral times
      scenario->instantiationTimes[initialization.instanceId] = BPMNOS::number(std::ceil((double)value));
    }

    // the disclosure time of a node is at least that of the scope containing it
    auto& nodeDisclosureTimes = scenario->disclosureTimes.at(initialization.instanceId);
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
  return scenario;
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

BPMNOS::Values StochasticDataProvider::getActivityReadyStatus(StaticDataProvider::Scenario& scenario, const Token* token) const {
  auto status = DynamicDataProvider::getActivityReadyStatus(scenario, token);
  apply(static_cast<Scenario&>(scenario), readyExpressions, (size_t)token->owner->root->instance.value(), token->node, status, *token->data, token->globals);
  return status;
}

BPMNOS::number StochasticDataProvider::getActivityReadyTime(const StaticDataProvider::Scenario& scenario, size_t instanceId, const BPMN::Node* activity, const BPMNOS::Values& readyStatus) const {
  // the token becomes ready once the timestamp of the status it becomes ready with is reached
  return std::max(DynamicDataProvider::getActivityReadyTime(scenario, instanceId, activity, readyStatus), readyStatus[BPMNOS::Model::ExtensionElements::Index::Timestamp].value());
}

BPMNOS::Values StochasticDataProvider::getCompletionStatus(StaticDataProvider::Scenario& scenario, const Token* token) const {
  auto status = DynamicDataProvider::getCompletionStatus(scenario, token);
  apply(static_cast<Scenario&>(scenario), completionExpressions, (size_t)token->owner->root->instance.value(), token->node, status, *token->data, token->globals);
  return status;
}

void StochasticDataProvider::apply(Scenario& scenario, const Expressions& expressions, size_t instanceId, const BPMN::Node* node, BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const {
  auto instanceExpressions = expressions.find(instanceId);
  if ( instanceExpressions == expressions.end() ) {
    return;
  }
  auto nodeExpressions = instanceExpressions->second.find(node);
  if ( nodeExpressions == instanceExpressions->second.end() ) {
    return;
  }
  randomDistributionFactory.setCurrentRng(&scenario.getRandomNumberGenerator(instanceId, node));
  for ( auto& expression : nodeExpressions->second ) {
    if ( auto value = expression->execute(status, data, globals) ) {
      auto target = expression->target.value();
      status[target->index] = convert(value.value(), target->type);
    }
  }
  randomDistributionFactory.setCurrentRng(nullptr);
}
