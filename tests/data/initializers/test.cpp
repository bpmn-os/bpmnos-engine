#include "prelude.h"
#include <limits>
#include <map>

namespace {

/// A model with a global object, a process with two status objects, a data object and a data attribute initialised
/// from a status object, a subprocess with a data object, and a task in it with a status object.
std::shared_ptr<const Model::Model> initializerModel() {
  std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
          <bpmnos:attribute id="Depot" name="depot" type="{ x: decimal, y: integer }" />
          <bpmnos:attribute id="Site" name="site" type="{ x: decimal }" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" name="instance" type="string" />
          <bpmnos:attribute id="Limit" name="limit := depot.x * 2" type="decimal" />
          <bpmnos:attribute id="Plan" name="plan" type="{ cost: decimal }" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="SubProcess_1" />
    <bpmn2:subProcess id="SubProcess_1">
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_2</bpmn2:outgoing>
      <bpmn2:dataObject id="DataObject_2">
        <bpmn2:extensionElements>
          <bpmnos:attributes>
            <bpmnos:attribute id="Budget" name="budget" type="decimal[2]" />
          </bpmnos:attributes>
        </bpmn2:extensionElements>
      </bpmn2:dataObject>
      <bpmn2:startEvent id="Start_2">
        <bpmn2:outgoing>Flow_3</bpmn2:outgoing>
      </bpmn2:startEvent>
      <bpmn2:sequenceFlow id="Flow_3" sourceRef="Start_2" targetRef="Task_1" />
      <bpmn2:task id="Task_1">
        <bpmn2:extensionElements>
          <bpmnos:status>
            <bpmnos:attributes>
              <bpmnos:attribute id="Route" name="route" type="integer[2]" />
            </bpmnos:attributes>
          </bpmnos:status>
        </bpmn2:extensionElements>
        <bpmn2:incoming>Flow_3</bpmn2:incoming>
        <bpmn2:outgoing>Flow_4</bpmn2:outgoing>
      </bpmn2:task>
      <bpmn2:sequenceFlow id="Flow_4" sourceRef="Task_1" targetRef="End_2" />
      <bpmn2:endEvent id="End_2">
        <bpmn2:incoming>Flow_4</bpmn2:incoming>
      </bpmn2:endEvent>
    </bpmn2:subProcess>
    <bpmn2:sequenceFlow id="Flow_2" sourceRef="SubProcess_1" targetRef="End_1" />
    <bpmn2:endEvent id="End_1">
      <bpmn2:incoming>Flow_2</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
  <bpmn2:dataStore id="DataStore_1">
    <bpmn2:extensionElements>
      <bpmnos:attributes>
        <bpmnos:attribute id="Location" name="location" type="integer[2]" />
      </bpmnos:attributes>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
</bpmn2:definitions>
)";
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

const BPMN::Node* nodeOf(const Model::Model& model, const std::string& id) {
  auto& process = model.processes.front();
  if ( process->id == id ) {
    return process.get();
  }
  return process->find([&id](BPMN::Node* node) { return node->id == id; });
}

/// A data provider giving access to the objects it creates.
template <typename Provider>
struct ObjectProvider : Provider {
  using Provider::Provider;

  /// Returns the object the instance data give an object a node declares, or the default object of the model.
  std::shared_ptr<const BPMNOS::Object> objectOf(const Execution::Scenario& scenario, size_t instanceId, const BPMN::Node* node, const std::string& name) const {
    auto object = node->extensionElements->as<Model::ExtensionElements>()->attributeRegistry[name];
    auto& objects = this->getInstanceObjects(static_cast<const Execution::StaticDataProvider::Scenario&>(scenario), instanceId);
    auto it = objects.find(object);
    return it != objects.end() ? it->second : this->getDefaultObject(object);
  }
};

using StaticProvider = ObjectProvider<Execution::StaticDataProvider>;
using ExpectedValueProvider = ObjectProvider<Execution::ExpectedValueDataProvider>;
using StochasticProvider = ObjectProvider<Execution::StochasticDataProvider>;

/// Records, for the states busy and completed of every node, the objects of the given names the token holds, rendered
/// as text, and keeps the object of the process token, so that it can be inspected after the run. A token holds the
/// objects of its scope only once the scope instance has come into being, which is not the case before it is ready.
struct ObjectRecorder : Execution::Observer {
  std::map<std::pair<std::string, std::string>, std::map<std::string, std::string>> objects;
  std::shared_ptr<const BPMNOS::Object> processDepot;
  void notice(const Execution::Observable* observable) override {
    auto token = static_cast<const Execution::Token*>(observable);
    auto extensionElements = token->node->extensionElements ? token->node->extensionElements->represents<Model::ExtensionElements>() : nullptr;
    if ( !extensionElements ) {
      return;
    }
    auto& registry = extensionElements->attributeRegistry;
    if ( token->node->represents<BPMN::Process>() ) {
      if ( token->state == Execution::Token::State::BUSY ) {
        processDepot = registry.getObject(registry["depot"], token->status, *token->data);
      }
      return;
    }
    if ( token->state != Execution::Token::State::BUSY && token->state != Execution::Token::State::COMPLETED ) {
      return;
    }
    auto& recorded = objects[{ token->node->id, Execution::Token::stateName[(int)token->state] }];
    for ( auto name : { "depot", "route", "budget" } ) {
      if ( registry.contains(name) && registry[name]->isObject() ) {
        recorded[name] = registry.getObject(registry[name], token->status, *token->data)->stringify();
      }
    }
  }
};

auto instance1 = []() { return (size_t)stringRegistry("Instance_1"); };

} // namespace

TEST_CASE( "Object initializers of the instance data", "[data][initializers]" ) {
  auto model = initializerModel();
  auto process = nodeOf(*model, "Process_1");

  SECTION( "Expression members are evaluated and converted to the types of their positions" ) {
    auto provider = std::make_shared<StaticProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; location := [ 2 * 2, 3 ]\n"
      "Instance_1; Process_1; depot := { x := pow(2, 2) + 0.5, y := 3 - 0.3 }\n"
      "Instance_1; Process_1; site := { x := depot.x + 1 }\n"
    );
    auto scenario = provider->createScenario();
    REQUIRE( provider->getGlobals(*scenario).objects[0]->stringify() == "[ 4, 3 ]" );
    REQUIRE( provider->objectOf(*scenario, instance1(), process, "depot")->stringify() == "{ x := 4.5, y := 2 }" );
    REQUIRE( provider->objectOf(*scenario, instance1(), process, "site")->stringify() == "{ x := 5.5 }" );
  }

  SECTION( "An initial value of the model reads an object an initializer gives" ) {
    auto provider = std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; depot := { x := 1 + 2, y := 0 }\n"
    );
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);
    engine.run(provider->createScenario());
    auto log = recorder.find(nlohmann::json{{"processId", "Process_1"}, {"state", "BUSY"}}, nlohmann::json{{"nodeId", nullptr}, {"event", nullptr}, {"decision", nullptr}});
    REQUIRE( log.size() == 1 );
    REQUIRE( log[0]["data"]["limit"] == 6 );
  }

  SECTION( "A member reading an object of a later row is refused" ) {
    REQUIRE_THROWS_WITH(
      std::make_shared<Execution::StaticDataProvider>(model,
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; site := { x := depot.x }\n"
        "Instance_1; Process_1; depot := { x := 1.0, y := 2 }\n"
      ),
      Catch::Matchers::ContainsSubstring("refers to object 'depot' without a value")
    );
  }

  SECTION( "A member reading another member of the same initializer is refused" ) {
    REQUIRE_THROWS(
      std::make_shared<Execution::StaticDataProvider>(model,
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; depot := { x := 1.0, y := x }\n"
      )
    );
  }

  SECTION( "A constant member is typed as it is written" ) {
    REQUIRE_THROWS_WITH(
      std::make_shared<Execution::StaticDataProvider>(model,
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; depot := { x := 1, y := 2 }\n"
      ),
      Catch::Matchers::ContainsSubstring("cannot be converted")
    );
  }

  SECTION( "The expected value data provider evaluates members with expected values" ) {
    auto provider = std::make_shared<ExpectedValueProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; depot := { x := uniform(0, 10), y := 2 }\n"
    );
    auto scenario = provider->createScenario();
    REQUIRE( provider->objectOf(*scenario, instance1(), process, "depot")->stringify() == "{ x := 5.0, y := 2 }" );
  }

  SECTION( "The dynamic data provider discloses an object with its row" ) {
    auto provider = std::make_shared<Execution::DynamicDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; depot := { x := 1.0, y := 2 }; 3\n"
    );
    REQUIRE( provider->getEarliestInstantiationTime(*provider->createScenario()) == 3 );
  }
}

TEST_CASE( "Object initializers sampled by the stochastic data provider", "[data][initializers]" ) {
  auto model = initializerModel();
  auto process = nodeOf(*model, "Process_1");
  const std::string csv =
    "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
    "Instance_1; Process_1; depot := { x := uniform(0, 100), y := 2 }; 0\n"
    "Instance_1; Process_1; site := { x := depot.x + uniform(0, 100) }; 10\n";

  auto depotAndSite = [&](StochasticProvider& provider, const Execution::Scenario& scenario) {
    return std::pair{ provider.objectOf(scenario, instance1(), process, "depot")->stringify(), provider.objectOf(scenario, instance1(), process, "site")->stringify() };
  };

  SECTION( "An initializer is sampled for each scenario and reproducibly for one seed" ) {
    auto provider = std::make_shared<StochasticProvider>(model, csv, 7);
    auto again = std::make_shared<StochasticProvider>(model, csv, 7);
    auto first = depotAndSite(*provider, *provider->createScenario(0));
    REQUIRE( first == depotAndSite(*again, *again->createScenario(0)) );
    REQUIRE( first.first != depotAndSite(*provider, *provider->createScenario(1)).first );
  }

  SECTION( "A fork keeps an object disclosed before the spawn time and samples one disclosed after it anew" ) {
    auto provider = std::make_shared<StochasticProvider>(model, csv, 7);
    auto scenario = provider->createScenario(0);
    static_cast<Execution::StaticDataProvider::Scenario&>(*scenario).time = 5;
    auto fork = provider->forkScenario(*scenario, 0);
    auto original = depotAndSite(*provider, *scenario);
    auto forked = depotAndSite(*provider, *fork);
    REQUIRE( forked.first == original.first );
    REQUIRE( forked.second != original.second );
  }

  SECTION( "A member reading an object of a later row is refused" ) {
    REQUIRE_THROWS_WITH(
      std::make_shared<StochasticProvider>(model,
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; site := { x := depot.x }\n"
        "Instance_1; Process_1; depot := { x := 1.0, y := 2 }\n"
      ),
      Catch::Matchers::ContainsSubstring("refers to object 'depot' without a value")
    );
  }
}

TEST_CASE( "Ready and completion expressions assign objects", "[data][initializers]" ) {
  auto model = initializerModel();
  const std::string head =
    "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
    "Instance_1; Process_1; depot := { x := 1.0, y := 2 };;;\n";

  auto run = [&model](const std::string& csv) {
    auto provider = std::make_shared<Execution::StochasticDataProvider>(model, csv, 7);
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    auto recorder = std::make_shared<ObjectRecorder>();
    engine.addSubscriber(recorder.get(), Execution::Observable::Type::Token);
    engine.run(provider->createScenario());
    return recorder;
  };

  SECTION( "A ready expression assigns a field of an inherited status object, an element of an owned data object, and a completion expression an element of a status object" ) {
    auto recorder = run(head +
      "Instance_1; SubProcess_1;;; budget[1] := 4;\n"
      "Instance_1; Task_1;;; depot.x := 7; route[1] := 9\n"
    );
    auto& busy = recorder->objects.at({ "Task_1", "BUSY" });
    REQUIRE( busy.at("depot") == "{ x := 7.0, y := 2 }" );
    REQUIRE( busy.at("budget") == "[ 4.0, undefined ]" );
    REQUIRE( busy.at("route") == "[ undefined, undefined ]" );
    REQUIRE( recorder->objects.at({ "Task_1", "COMPLETED" }).at("route") == "[ 9, undefined ]" );
    // the object the process token holds is copied before the token at the task writes it
    REQUIRE( recorder->processDepot->stringify() == "{ x := 1.0, y := 2 }" );
  }

  SECTION( "A completion expression assigning an element of a status object is evaluated anew in an installed system state" ) {
    const std::string csv = head +
      "Instance_1; Task_1;;;; timestamp := timestamp + uniform(10, 1000)\n"
      "Instance_1; Task_1;;;; route[1] := uniform_int(1, 1000000)\n";
    auto provider = std::make_shared<Execution::StochasticDataProvider>(model, csv, 7);

    // the route the task completes with when a scenario is run from the given state, or from the start
    auto completedRoute = [&](std::unique_ptr<Execution::Scenario> scenario, const Execution::SystemState* state) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      ObjectRecorder recorder;
      engine.addSubscriber(&recorder, Execution::Observable::Type::Token);
      if ( state ) {
        engine.initializeSystemState(std::move(scenario), state);
        engine.resume();
      }
      else {
        engine.run(std::move(scenario));
      }
      return recorder.objects.at({ "Task_1", "COMPLETED" }).at("route");
    };

    // the run stops while the task is busy, awaiting its completion
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    provider->setEndTime(5);
    engine.run(provider->createScenario());
    REQUIRE( engine.getSystemState()->getTime() == 5 );
    provider->setEndTime(std::numeric_limits<BPMNOS::number>::max());

    auto runRoute = completedRoute(provider->createScenario(), nullptr);
    auto forkRoute = completedRoute(provider->forkScenario(*engine.getSystemState()->scenario, 0), engine.getSystemState());
    REQUIRE( runRoute.ends_with(", undefined ]") );
    REQUIRE( forkRoute.ends_with(", undefined ]") );
    REQUIRE( forkRoute != runRoute );
  }

  SECTION( "A ready expression assigning a field of an inherited data object is refused" ) {
    REQUIRE_THROWS_WITH(
      std::make_shared<Execution::StochasticDataProvider>(model, head + "Instance_1; Task_1;;; plan.cost := 5;\n", 7),
      Catch::Matchers::ContainsSubstring("must assign a status attribute or a data attribute the scope owns")
    );
  }

  SECTION( "A completion expression assigning a data object is refused" ) {
    REQUIRE_THROWS_WITH(
      std::make_shared<Execution::StochasticDataProvider>(model, head + "Instance_1; Task_1;;;; plan.cost := 5\n", 7),
      Catch::Matchers::ContainsSubstring("must assign a status attribute")
    );
  }
}
