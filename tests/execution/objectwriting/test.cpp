#include "prelude.h"
#include <map>

namespace {

std::shared_ptr<const Model::Model> modelFrom(const std::string& xml) {
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

const std::string header = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
)";

/// A model with a global array of fixed size, a status array, and data objects of several shapes.
std::shared_ptr<const Model::Model> writingModel() {
  return modelFrom(header + R"(
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
          <bpmnos:attribute id="Route" name="route" type="integer[]" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" name="instance" type="string" />
          <bpmnos:attribute id="Facilities" name="facilities" type="{ cost: decimal, flags: boolean[] }[]" />
          <bpmnos:attribute id="Grid" name="grid" type="integer[2][3]" />
          <bpmnos:attribute id="Visits" name="visits" type="integer[]" />
          <bpmnos:attribute id="Budget" name="budget" type="decimal" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1" />
  </bpmn2:process>
  <bpmn2:dataStore id="DataStore_1">
    <bpmn2:extensionElements>
      <bpmnos:attributes>
        <bpmnos:attribute id="Location" name="location" type="integer[3]" />
      </bpmnos:attributes>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
</bpmn2:definitions>
)");
}

/// A static data provider giving access to the statuses and data it creates.
struct WritingProvider : Execution::StaticDataProvider {
  using StaticDataProvider::StaticDataProvider;
  using StaticDataProvider::getStatus;
  using StaticDataProvider::getData;
};

const Model::AttributeRegistry& registryOf(const Model::Model& model) {
  return model.processes.front()->extensionElements->as<Model::ExtensionElements>()->attributeRegistry;
}

/// Records a copy of the status and data of every token at every node and state.
struct Capture : Execution::Observer {
  std::map<std::pair<std::string, std::string>, std::pair<BPMNOS::Status, BPMNOS::Data>> states;
  std::vector< std::vector<const Model::Attribute*> > updates;
  void notice(const Execution::Observable* observable) override {
    if ( observable->getObservableType() == Execution::Observable::Type::DataUpdate ) {
      updates.push_back( static_cast<const Execution::DataUpdate*>(observable)->attributes );
      return;
    }
    auto token = static_cast<const Execution::Token*>(observable);
    auto node = token->node->represents<BPMN::FlowNode>() ? token->node->id : std::string("process");
    states[{node, Execution::Token::stateName[(int)token->state]}] = { token->status, BPMNOS::Data(*token->data) };
  }
};

} // namespace

TEST_CASE( "Write objects, elements and lengths", "[execution][objects][writing]" ) {
  auto model = writingModel();
  auto provider = std::make_shared<WritingProvider>(model,
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "; ; location := [1, 2]\n"
    "Instance_1; Process_1; route := [1, 2]\n"
    "Instance_1; Process_1; facilities := [ { cost := 1 }, { cost := 2 } ]\n"
    "Instance_1; Process_1; grid := [ [1, 2, 3], [4, 5, 6] ]\n"
    "Instance_1; Process_1; visits := [9]\n"
  );
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

  auto write = [&](const std::string& text) {
    Model::Expression expression(InputEncoder(text), registry);
    REQUIRE( expression.writesObject() );
    expression.write(status, data);
  };
  auto read = [&](const std::string& text) {
    Model::Expression expression(InputEncoder(text), registry);
    return expression.execute(status, data);
  };
  auto object = [&](const std::string& name) {
    return registry.getObject(registry[name], status, data);
  };
  auto render = [&](const std::string& name) {
    return to_string(*object(name));
  };

  SECTION( "Lengths not yet determined are empty" ) {
    REQUIRE( render("facilities") == "[ { cost := 1, flags := [ ] }, { cost := 2, flags := [ ] } ]" );
  }

  SECTION( "A whole assignment pads a fixed dimension and takes the length of an open one" ) {
    write("location := [7]");
    REQUIRE( render("location") == "[ 7, undefined, undefined ]" );
    REQUIRE_THROWS_WITH( write("location := [1, 2, 3, 4]"), Catch::Matchers::ContainsSubstring("4 elements where 3 are declared") );
    write("route := [4, 5, 6]");
    REQUIRE( render("route") == "[ 4, 5, 6 ]" );
  }

  SECTION( "An object of the same layout is shared, a part of an object copied" ) {
    write("visits := route");
    REQUIRE( object("visits").get() == object("route").get() );
    write("route := grid[2]");
    REQUIRE( render("route") == "[ 4, 5, 6 ]" );
    REQUIRE( render("visits") == "[ 1, 2 ]" );
  }

  SECTION( "Values are written to elements" ) {
    write("grid[1][2] := 9");
    REQUIRE( render("grid") == "[ [ 1, 9, 3 ], [ 4, 5, 6 ] ]" );
    write("grid[1][2] += 1");
    REQUIRE( read("grid[1][2]") == 10 );
    write("grid[1][3] := location[3]");
    REQUIRE( render("grid") == "[ [ 1, 10, undefined ], [ 4, 5, 6 ] ]" );
    REQUIRE_THROWS_WITH( write("grid[3][1] := 1"), Catch::Matchers::ContainsSubstring("illegal index") );
  }

  SECTION( "An array or object written to an element keeps its lengths" ) {
    write("grid[2] := [7]");
    REQUIRE( render("grid") == "[ [ 1, 2, 3 ], [ 7, undefined, undefined ] ]" );
    REQUIRE_THROWS_WITH( write("grid[2] := [1, 2, 3, 4]"), Catch::Matchers::ContainsSubstring("4 elements where 3 are declared") );
    write("facilities[1] := { cost := 5 }");
    REQUIRE( render("facilities") == "[ { cost := 5, flags := [ ] }, { cost := 2, flags := [ ] } ]" );
  }

  SECTION( "The first assignment to a nested element determines its length for every element" ) {
    write("facilities[1].flags := [true, false]");
    REQUIRE( render("facilities") == "[ { cost := 1, flags := [ true, false ] }, { cost := 2, flags := [ undefined, undefined ] } ]" );
    REQUIRE( read("size(facilities[2].flags)") == 2 );
    REQUIRE( read("count(facilities[2].flags)") == 0 );
    REQUIRE_THROWS_WITH( write("facilities[2].flags := [true, false, true]"), Catch::Matchers::ContainsSubstring("3 elements where 2 are declared") );
  }

  SECTION( "Resize grows and shrinks a dimension, nested ones in every element" ) {
    write("resize(route) := 4");
    REQUIRE( render("route") == "[ 1, 2, undefined, undefined ]" );
    REQUIRE( read("size(route)") == 4 );
    REQUIRE( read("count(route)") == 2 );
    write("resize(route) := 1");
    REQUIRE( render("route") == "[ 1 ]" );
    write("resize(facilities[2].flags) := 2");
    REQUIRE( render("facilities") == "[ { cost := 1, flags := [ undefined, undefined ] }, { cost := 2, flags := [ undefined, undefined ] } ]" );
    write("resize(grid[1]) := 2");
    REQUIRE( render("grid") == "[ [ 1, 2 ], [ 4, 5 ] ]" );
    write("resize(location) := 4");
    REQUIRE( read("size(location)") == 4 );
    // the dimension stays fixed
    write("location := [1]");
    REQUIRE( render("location") == "[ 1, undefined, undefined, undefined ]" );
    REQUIRE_THROWS_WITH( write("resize(route) := 0 - 1"), Catch::Matchers::ContainsSubstring("non-negative integer") );
  }

  SECTION( "A write to a shared object copies it" ) {
    auto copy = status;
    REQUIRE( copy.objects[0].get() == status.objects[0].get() );
    Model::Expression expression(InputEncoder("route[1] := 8"), registry);
    expression.write(copy, data);
    REQUIRE( to_string(*copy.objects[0]) == "[ 8, 2 ]" );
    REQUIRE( render("route") == "[ 1, 2 ]" );
    // an object held by no one else is written in place
    auto written = copy.objects[0].get();
    expression.write(copy, data);
    REQUIRE( copy.objects[0].get() == written );
  }

  SECTION( "A write through a data chain changes the data of the scope" ) {
    SharedData chain(data);
    Model::Expression expression(InputEncoder("visits[1] := 3"), registry);
    expression.write(status, chain);
    REQUIRE( render("visits") == "[ 3 ]" );
  }
}

TEST_CASE( "Refuse illegal assignments to objects", "[execution][objects][writing]" ) {
  auto model = writingModel();
  auto& registry = registryOf(*model);
  auto compile = [&registry](const std::string& text) {
    Model::Expression expression(InputEncoder(text), registry);
  };

  REQUIRE_THROWS_WITH( compile("route := 5"), Catch::Matchers::ContainsSubstring("must be assigned a literal") );
  REQUIRE_THROWS_WITH( compile("route := grid[1][1]"), Catch::Matchers::ContainsSubstring("must be assigned a literal") );
  REQUIRE_THROWS_WITH( compile("route := budget"), Catch::Matchers::ContainsSubstring("must be assigned a literal") );
  REQUIRE_THROWS_WITH( compile("route += [1]"), Catch::Matchers::ContainsSubstring("must be assigned by ':='") );
  REQUIRE_THROWS_WITH( compile("route := undefined"), Catch::Matchers::ContainsSubstring("cannot be undefined") );
  REQUIRE_THROWS_WITH( compile("resize(grid[1][1]) := 2"), Catch::Matchers::ContainsSubstring("is not an array") );
  REQUIRE_THROWS_WITH( compile("resize(budget) := 2"), Catch::Matchers::ContainsSubstring("resize requires an array") );
  REQUIRE_THROWS_WITH( compile("budget[1] := 2"), Catch::Matchers::ContainsSubstring("which is not an object") );
  REQUIRE_THROWS_WITH( compile("facilities.cost := 2"), Catch::Matchers::ContainsSubstring("dimensions must be indexed first") );
  REQUIRE_NOTHROW( compile("facilities[1].flags[2] := facilities[2].cost > 1") );
}

namespace {

/// The Quadratic Assignment Problem of three facilities: a task assigns each facility a location, and the
/// cost sums the flows between facilities times the distances between their locations.
std::shared_ptr<const Model::Model> assignmentModel() {
  return modelFrom(header + R"(
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
          <bpmnos:attribute id="Cost" name="cost" type="decimal" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="Task_1" />
    <bpmn2:task id="Task_1">
      <bpmn2:extensionElements>
        <bpmnos:status>
          <bpmnos:operators>
            <bpmnos:operator id="Operator_1" expression="location[1] := 2" />
            <bpmnos:operator id="Operator_2" expression="location[2] := 3" />
            <bpmnos:operator id="Operator_3" expression="location[3] := 1" />
            <bpmnos:operator id="Operator_4" expression="cost := sum{ flow[i][j] * distance[location[i]][location[j]] | i in 1..3, j in 1..3 }" />
          </bpmnos:operators>
        </bpmnos:status>
      </bpmn2:extensionElements>
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
        <bpmnos:attribute id="Flow" name="flow" type="decimal[3][3]" />
        <bpmnos:attribute id="Distance" name="distance" type="decimal[3][3]" />
      </bpmnos:attributes>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
</bpmn2:definitions>
)");
}

/// A model whose parallel branches write a status array before they are joined.
std::shared_ptr<const Model::Model> parallelModel(const std::string& operatorsA, const std::string& operatorsB) {
  auto task = [](const std::string& id, const std::string& operators, const std::string& incoming, const std::string& outgoing) {
    std::string extensionElements = operators.empty() ? "" : R"(
      <bpmn2:extensionElements>
        <bpmnos:status>
          <bpmnos:operators>)" + operators + R"(</bpmnos:operators>
        </bpmnos:status>
      </bpmn2:extensionElements>)";
    return R"(
    <bpmn2:task id=")" + id + R"(">)" + extensionElements + R"(
      <bpmn2:incoming>)" + incoming + R"(</bpmn2:incoming>
      <bpmn2:outgoing>)" + outgoing + R"(</bpmn2:outgoing>
    </bpmn2:task>)";
  };
  return modelFrom(header + R"(
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
          <bpmnos:attribute id="Route" name="route" type="integer[]" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" name="instance" type="string" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="Split" />
    <bpmn2:parallelGateway id="Split">
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_A1</bpmn2:outgoing>
      <bpmn2:outgoing>Flow_B1</bpmn2:outgoing>
    </bpmn2:parallelGateway>
    <bpmn2:sequenceFlow id="Flow_A1" sourceRef="Split" targetRef="Task_A" />
    <bpmn2:sequenceFlow id="Flow_B1" sourceRef="Split" targetRef="Task_B" />)"
    + task("Task_A", operatorsA, "Flow_A1", "Flow_A2")
    + task("Task_B", operatorsB, "Flow_B1", "Flow_B2") + R"(
    <bpmn2:sequenceFlow id="Flow_A2" sourceRef="Task_A" targetRef="Join" />
    <bpmn2:sequenceFlow id="Flow_B2" sourceRef="Task_B" targetRef="Join" />
    <bpmn2:parallelGateway id="Join">
      <bpmn2:incoming>Flow_A2</bpmn2:incoming>
      <bpmn2:incoming>Flow_B2</bpmn2:incoming>
      <bpmn2:outgoing>Flow_2</bpmn2:outgoing>
    </bpmn2:parallelGateway>
    <bpmn2:sequenceFlow id="Flow_2" sourceRef="Join" targetRef="End_1" />
    <bpmn2:endEvent id="End_1">
      <bpmn2:incoming>Flow_2</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
</bpmn2:definitions>
)");
}

/// Runs a model for the given instance data, recording what the tokens hold.
void run(const std::shared_ptr<const Model::Model>& model, const std::string& csv, Capture& capture) {
  auto provider = std::make_shared<Execution::StaticDataProvider>(model, csv);
  Execution::Engine engine(model);
  Execution::InstantEntry entryHandler;
  Execution::InstantExit exitHandler;
  entryHandler.connect(&engine);
  exitHandler.connect(&engine);
  engine.addSubscriber(&capture, Execution::Observable::Type::Token, Execution::Observable::Type::DataUpdate);
  engine.run(provider->createScenario());
}

std::string operatorText(const std::string& expression) {
  static size_t counter = 0;
  return "<bpmnos:operator id=\"Operator_" + std::to_string(++counter) + "\" expression=\"" + expression + "\" />";
}

} // namespace

TEST_CASE( "Operators write the locations of the Quadratic Assignment Problem", "[execution][objects][writing]" ) {
  auto model = assignmentModel();
  Capture capture;
  run(model,
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "; ; flow := [ [0, 1, 2], [1, 0, 3], [2, 3, 0] ]\n"
    "; ; distance := [ [0, 5, 6], [5, 0, 7], [6, 7, 0] ]\n"
    "Instance_1; Process_1;\n",
    capture
  );
  auto& [status, data] = capture.states.at({"End_1", "DONE"});
  auto& registry = registryOf(*model);
  REQUIRE( to_string(*registry.getObject(registry["location"], status, data)) == "[ 2, 3, 1 ]" );
  REQUIRE( registry.getValue(registry["cost"], status, data) == 70 );
  // the update of the task names the global object written
  REQUIRE( std::ranges::any_of(capture.updates, [&registry](auto& attributes) { return std::ranges::contains(attributes, registry["location"]); }) );
}

TEST_CASE( "Status objects merge at a join", "[execution][objects][writing]" ) {
  const std::string csv =
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "Instance_1; Process_1; route := [1, 2]\n";
  auto merged = [&csv](const std::string& operatorsA, const std::string& operatorsB) {
    auto model = parallelModel(operatorsA, operatorsB);
    Capture capture;
    run(model, csv, capture);
    auto& [status, data] = capture.states.at({"End_1", "DONE"});
    auto& registry = registryOf(*model);
    return to_string(*registry.getObject(registry["route"], status, data));
  };

  SECTION( "Values that agree are kept" ) {
    REQUIRE( merged(operatorText("route[1] := 5"), operatorText("route[1] := 5")) == "[ 5, 2 ]" );
  }
  SECTION( "A conflict makes a value undefined" ) {
    REQUIRE( merged(operatorText("route[1] := 5"), "") == "[ undefined, 2 ]" );
  }
  SECTION( "An element only one branch has is taken from it" ) {
    REQUIRE( merged(operatorText("resize(route) := 3") + operatorText("route[3] := 7"), "") == "[ 1, 2, 7 ]" );
  }
}
