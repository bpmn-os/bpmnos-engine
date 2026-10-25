#include "DataProvider.h"
#include "Scenario.h"
#include "execution/engine/src/SystemState.h"
#include "execution/engine/src/events/ClockTickEvent.h"
#include "model/bpmnos/src/extensionElements/ExtensionElements.h"

using namespace BPMNOS::Execution;

DataProvider::DataProvider(std::shared_ptr<const BPMNOS::Model::Model> model, std::chrono::milliseconds clockTickDuration)
  : clockTickDuration(clockTickDuration)
  , model(std::move(model))
{
  // the content of a message, signal, error or escalation starts from an undefined object of its type, every node of
  // a signal sharing one; a data provider declaring sizes replaces these defaults
  for ( auto& process : this->model->processes ) {
    for ( auto node : process->find_all([](const BPMN::Node* node) { return node->extensionElements && node->extensionElements->represents<const BPMNOS::Model::ExtensionElements>() && node->extensionElements->as<const BPMNOS::Model::ExtensionElements>()->content; }) ) {
      auto extensionElements = node->extensionElements->as<const BPMNOS::Model::ExtensionElements>();
      auto content = extensionElements->content.get();
      if ( auto signalDefinition = extensionElements->signalDefinition.get() ) {
        auto [it, _] = defaultContents.try_emplace(signalDefinition->name, content->type.undefinedObject());
        defaultObjects[content] = it->second;
      }
      else {
        defaultObjects[content] = content->type.undefinedObject();
      }
    }
  }
}

std::unique_ptr<Scenario> DataProvider::forkScenario([[maybe_unused]] const Scenario& scenario, [[maybe_unused]] unsigned int index) const {
  // the future of the run is certain, so that its fork is a new scenario
  return createScenario();
}

const std::shared_ptr<const BPMNOS::Model::Model>& DataProvider::getModel() const {
  return model;
}

const std::shared_ptr<const BPMNOS::Object>& DataProvider::getDefaultObject(const BPMNOS::Model::Attribute* object) const {
  auto it = defaultObjects.find(object);
  if ( it == defaultObjects.end() ) {
    throw std::logic_error("DataProvider: object '" + object->id + "' has no default");
  }
  return it->second;
}

const std::shared_ptr<const BPMNOS::Object>& DataProvider::getDefaultContent(BPMNOS::number signalName) const {
  auto it = defaultContents.find(signalName);
  if ( it == defaultContents.end() ) {
    throw std::invalid_argument("DataProvider: the model states no signal '" + BPMNOS::to_string(signalName,STRING) + "'");
  }
  return it->second;
}

void DataProvider::advance(const SystemState* systemState, Scenario& scenario, EventQueue& queue) const {
  auto now = std::chrono::steady_clock::now();
  // the elapsed time is compared in milliseconds rather than the duration added to the previous clock tick,
  // so that the maximum duration, with which the data provider never advances time itself, cannot overflow
  if ( std::chrono::duration_cast<std::chrono::milliseconds>(now - scenario.previousClockTick) < clockTickDuration ) {
    // the clock tick is not yet due; the controller may still decide meanwhile
    return;
  }
  scenario.previousClockTick = now;
  queue.push_back(std::make_shared<ClockTickEvent>(systemState));
}
