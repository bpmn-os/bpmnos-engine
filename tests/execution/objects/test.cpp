#include "prelude.h"
#include <set>

namespace {

/// A model with global arrays, a process with a status array, data objects of several shapes and scalar data
/// given by the instance data, and a restriction reading a global array.
std::shared_ptr<const Model::Model> readingModel() {
  std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
          <bpmnos:attribute id="Route" name="route" type="integer[]" />
        </bpmnos:attributes>
        <bpmnos:restrictions>
          <bpmnos:restriction id="Restriction_1" expression="sum(location) &gt;= 3" />
        </bpmnos:restrictions>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" name="instance" type="string" />
          <bpmnos:attribute id="Facilities" name="facilities" type="{ cost: decimal, flags: boolean[2] }[]" />
          <bpmnos:attribute id="Grid" name="grid" type="integer[2][3]" />
          <bpmnos:attribute id="Depot" name="depot := { x := 1, y := 2 }" type="{ x: decimal, y: decimal }" />
          <bpmnos:attribute id="Budget" name="budget" type="decimal" />
          <bpmnos:attribute id="Offset" name="offset" type="decimal" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="Task_1" />
    <bpmn2:task id="Task_1">
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_2</bpmn2:outgoing>
    </bpmn2:task>
    <bpmn2:sequenceFlow id="Flow_2" sourceRef="Task_1" targetRef="End_1" />
    <bpmn2:endEvent id="End_1">
      <bpmn2:incoming>Flow_2</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
  <bpmn2:dataStore id="DataStore_1">
    <bpmn2:extensionElements>
      <bpmnos:attributes>
        <bpmnos:attribute id="Location" name="location" type="integer[3]" />
        <bpmnos:attribute id="Flow" name="flow" type="decimal[2][2]" />
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

/// A static data provider giving access to the statuses and data it creates.
struct ReadingProvider : Execution::StaticDataProvider {
  using StaticDataProvider::StaticDataProvider;
  using StaticDataProvider::getStatus;
  using StaticDataProvider::getData;
};

const std::string instanceData =
  "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
  "; ; location := [2, 1]\n"
  "; ; flow := [ [1, 2], [3, 4] ]\n"
  "; Process_1; facilities[3]\n"
  "Instance_1; Process_1; facilities := [ { cost := 10, flags := [ true, false ] }, { cost := 20, flags := [ false, false ] } ]\n"
  "Instance_1; Process_1; grid := [ [1, 2, 3], [4, 5, 6] ]\n"
  "Instance_1; Process_1; route := [1, 2]\n"
  "Instance_1; Process_1; budget := sum(location)\n"
  "Instance_1; Process_1; offset := depot.x + grid[2][1]\n";

const Model::AttributeRegistry& registryOf(const Model::Model& model) {
  return model.processes.front()->extensionElements->as<Model::ExtensionElements>()->attributeRegistry;
}

} // namespace

TEST_CASE( "Read objects in expressions", "[execution][objects]" ) {
  auto model = readingModel();
  auto provider = std::make_shared<ReadingProvider>(model, instanceData);
  auto scenario = provider->createScenario();
  auto& staticScenario = static_cast<const Execution::StaticDataProvider::Scenario&>(*scenario);
  auto instance = (size_t)stringRegistry("Instance_1");
  auto process = model->processes.front().get();
  auto& registry = registryOf(*model);

  // the data of the process follows the global data
  auto data = provider->getGlobals(*scenario);
  auto own = provider->getData(staticScenario, instance, process);
  data.attributes.insert(data.attributes.end(), own.attributes.begin(), own.attributes.end());
  data.objects.insert(data.objects.end(), own.objects.begin(), own.objects.end());
  auto status = provider->getStatus(staticScenario, instance, process);

  auto evaluate = [&](const std::string& text) {
    Model::Expression expression(InputEncoder::fragment(text), registry);
    return expression.execute(status, data);
  };

  SECTION( "Paths address values" ) {
    REQUIRE( evaluate("grid[2][3]") == 6 );
    REQUIRE( evaluate("facilities[2].cost") == 20 );
    REQUIRE( evaluate("facilities[1].flags[1]") == 1 );
    REQUIRE( evaluate("depot.y") == 2 );
    REQUIRE( evaluate("route[2]") == 2 );
    REQUIRE( evaluate("grid[1][location[2]]") == 1 );
  }

  SECTION( "Arrays are aggregated and tested for membership" ) {
    REQUIRE( evaluate("sum(grid[2])") == 15 );
    REQUIRE( evaluate("sum(location)") == 3 );
    REQUIRE( evaluate("2 in location") == 1 );
    REQUIRE( evaluate("3 in location") == 0 );
    REQUIRE( evaluate("max(facilities[1].flags)") == 1 );
  }

  SECTION( "Size counts every element and count the defined ones" ) {
    REQUIRE( evaluate("size(location)") == 3 );
    REQUIRE( evaluate("count(location)") == 2 );
    REQUIRE( evaluate("size(facilities)") == 3 );
    REQUIRE( evaluate("size(grid)") == 2 );
    REQUIRE( evaluate("size(grid[1])") == 3 );
  }

  SECTION( "Aggregations over expressions skip undefined values" ) {
    REQUIRE( evaluate("sum{ facilities[i].cost | i in 1..size(facilities) }") == 30 );
    REQUIRE( evaluate("sum{ facilities[i].cost | i in 1..2, facilities[i].flags[1] }") == 10 );
    REQUIRE( evaluate("sum{ flow[i][j] * grid[i][j] | i in 1..2, j in 1..2 }") == 37 );
    REQUIRE( evaluate("min{ d | d in location, d > 1 }") == 2 );
  }

  SECTION( "An undefined value makes the expression undefined" ) {
    REQUIRE( evaluate("location[3] + 1") == std::nullopt );
    REQUIRE( evaluate("location[3] < 5") == std::nullopt );
    REQUIRE( evaluate("facilities[3].cost") == std::nullopt );
  }

  SECTION( "An index outside the size of its dimension is an error" ) {
    REQUIRE_THROWS_WITH( evaluate("grid[3][1]"), Catch::Matchers::ContainsSubstring("illegal index") );
    REQUIRE_THROWS_WITH( evaluate("location[0]"), Catch::Matchers::ContainsSubstring("illegal index") );
  }

  SECTION( "Expressions of the instance data read objects given before and initialised by the model" ) {
    auto budget = registry["budget"];
    auto offset = registry["offset"];
    REQUIRE( registry.getValue(budget, status, data) == 3 );
    REQUIRE( registry.getValue(offset, status, data) == 5 );
  }
}

TEST_CASE( "Refuse illegal uses of objects in expressions", "[execution][objects]" ) {
  auto model = readingModel();
  auto& registry = registryOf(*model);
  auto compile = [&registry](const std::string& text) {
    Model::Expression expression(InputEncoder::fragment(text), registry);
  };

  REQUIRE_THROWS_WITH( compile("facilities.cost"), Catch::Matchers::ContainsSubstring("dimensions must be indexed first") );
  REQUIRE_THROWS_WITH( compile("grid[1][2][3]"), Catch::Matchers::ContainsSubstring("more indices") );
  REQUIRE_THROWS_WITH( compile("location + 1"), Catch::Matchers::ContainsSubstring("must be indexed to a value") );
  REQUIRE_THROWS_WITH( compile("facilities[1] + 1"), Catch::Matchers::ContainsSubstring("does not address a value") );
  REQUIRE_THROWS_WITH( compile("sum(grid)"), Catch::Matchers::ContainsSubstring("single dimension") );
  REQUIRE_THROWS_WITH( compile("sum(facilities)"), Catch::Matchers::ContainsSubstring("single dimension") );
  REQUIRE_THROWS_WITH( compile("depot.z"), Catch::Matchers::ContainsSubstring("unknown field") );
  REQUIRE_THROWS_WITH( compile("size(depot)"), Catch::Matchers::ContainsSubstring("is not an array") );
  REQUIRE_NOTHROW( compile("size(facilities[1].flags) + size(grid)") );
}

TEST_CASE( "Refuse an object read in the instance data before it has a value", "[execution][objects]" ) {
  auto model = readingModel();
  REQUIRE_THROWS_WITH(
    std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; location := [2, 1]\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Process_1; budget := size(route)\n"
      "Instance_1; Process_1; route := [1, 2]\n"
    ),
    Catch::Matchers::ContainsSubstring("refers to object 'route' without a value")
  );
}

TEST_CASE( "A restriction reading an object decides feasibility", "[execution][objects]" ) {
  auto model = readingModel();
  auto outcome = [&model](const std::string& location) {
    auto provider = std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; location := " + location + "\n"
      "; Process_1; facilities[1]\n"
      "Instance_1; Process_1; route := [1]\n"
    );
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::OutcomeSentinel sentinel;
    sentinel.subscribe(&engine);
    engine.run(provider->createScenario());
    return sentinel.getOutcome();
  };

  REQUIRE( outcome("[2, 1]") == Execution::Outcome::COMPLETED );
  REQUIRE( outcome("[1, 1]") == Execution::Outcome::FAILED );
}

namespace {

/// A model whose exclusive gateway routes a token by guards reading an array of objects.
std::shared_ptr<const Model::Model> gatewayModel() {
  std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" name="instance" type="string" />
          <bpmnos:attribute id="Facilities" name="facilities" type="{ cost: decimal }[1]" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="Gateway_1" />
    <bpmn2:exclusiveGateway id="Gateway_1">
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_Cheap</bpmn2:outgoing>
      <bpmn2:outgoing>Flow_Expensive</bpmn2:outgoing>
    </bpmn2:exclusiveGateway>
    <bpmn2:sequenceFlow id="Flow_Cheap" sourceRef="Gateway_1" targetRef="Task_Cheap">
      <bpmn2:extensionElements>
        <bpmnos:restrictions>
          <bpmnos:restriction id="Guard_Cheap" expression="facilities[1].cost &lt; 15" />
        </bpmnos:restrictions>
      </bpmn2:extensionElements>
    </bpmn2:sequenceFlow>
    <bpmn2:sequenceFlow id="Flow_Expensive" sourceRef="Gateway_1" targetRef="Task_Expensive">
      <bpmn2:extensionElements>
        <bpmnos:restrictions>
          <bpmnos:restriction id="Guard_Expensive" expression="facilities[1].cost &gt;= 15" />
        </bpmnos:restrictions>
      </bpmn2:extensionElements>
    </bpmn2:sequenceFlow>
    <bpmn2:task id="Task_Cheap">
      <bpmn2:incoming>Flow_Cheap</bpmn2:incoming>
      <bpmn2:outgoing>Flow_2</bpmn2:outgoing>
    </bpmn2:task>
    <bpmn2:task id="Task_Expensive">
      <bpmn2:incoming>Flow_Expensive</bpmn2:incoming>
      <bpmn2:outgoing>Flow_3</bpmn2:outgoing>
    </bpmn2:task>
    <bpmn2:sequenceFlow id="Flow_2" sourceRef="Task_Cheap" targetRef="Gateway_2" />
    <bpmn2:sequenceFlow id="Flow_3" sourceRef="Task_Expensive" targetRef="Gateway_2" />
    <bpmn2:exclusiveGateway id="Gateway_2">
      <bpmn2:incoming>Flow_2</bpmn2:incoming>
      <bpmn2:incoming>Flow_3</bpmn2:incoming>
      <bpmn2:outgoing>Flow_4</bpmn2:outgoing>
    </bpmn2:exclusiveGateway>
    <bpmn2:sequenceFlow id="Flow_4" sourceRef="Gateway_2" targetRef="End_1" />
    <bpmn2:endEvent id="End_1">
      <bpmn2:incoming>Flow_4</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
</bpmn2:definitions>
)";
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

} // namespace

TEST_CASE( "Guards reading an object route a token at an exclusive gateway", "[execution][objects]" ) {
  auto model = gatewayModel();
  // returns the tasks a token entered
  auto route = [&model](const std::string& cost) {
    auto provider = std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; facilities := [ { cost := " + cost + " } ]\n"
    );
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);
    engine.run(provider->createScenario());
    std::set<std::string> tasks;
    for ( auto& entry : recorder.find(nlohmann::json{{"state","ENTERED"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}}) ) {
      if ( entry.contains("nodeId") && entry["nodeId"].get<std::string>().starts_with("Task_") ) {
        tasks.insert(entry["nodeId"].get<std::string>());
      }
    }
    return tasks;
  };

  REQUIRE( route("10") == std::set<std::string>{ "Task_Cheap" } );
  REQUIRE( route("20") == std::set<std::string>{ "Task_Expensive" } );
}
