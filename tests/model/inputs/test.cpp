#include "prelude.h"
#include <map>

namespace {

/// The Quadratic Assignment Problem of three facilities with the flows and distances given as matrix inputs, a
/// table of depots given as a JSON object input, and a task assigning each facility a location.
std::string inputModel(const std::string& inputs, const std::string& attributes = "") {
  return R"xml(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
          <bpmnos:attribute id="Row" name="row" type="decimal[]" />
)xml" + attributes + R"xml(
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
      </bpmnos:attributes>
      <bpmnos:inputs>
)xml" + inputs + R"xml(
      </bpmnos:inputs>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
</bpmn2:definitions>
)xml";
}

const std::string standardInputs = R"xml(
        <bpmnos:input id="Input_1" name="flow" type="matrix" source="flow.csv" schema="decimal[3][3]" />
        <bpmnos:input id="Input_2" name="distance" type="matrix" source="distance.csv" schema="decimal[][]" />
        <bpmnos:input id="Input_3" name="depots" type="object" source="depots.json" schema="{ x: decimal, name: string }[]" />
)xml";

const std::unordered_map<std::string, std::string> standardContents = {
  { "flow.csv", "0;1;2\n1;0;3\n2;3;0\n" },
  { "distance.csv", "0;5;6\n5;0;7\n6;7;0\n" },
  { "depots.json", R"([ { "x": 1, "name": "A" }, { "x": 2 } ])" }
};

std::shared_ptr<const Model::Model> modelFrom(const std::string& xml, const std::unordered_map<std::string, std::string>& contents) {
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), contents);
}

/// Returns the model with the given inputs, one of whose contents is replaced.
std::shared_ptr<const Model::Model> modelWith(const std::string& inputs, const std::string& source, const std::string& content) {
  auto contents = standardContents;
  contents[source] = content;
  return modelFrom(inputModel(inputs), contents);
}

const Model::AttributeRegistry& registryOf(const Model::Model& model) {
  return model.processes.front()->extensionElements->as<Model::ExtensionElements>()->attributeRegistry;
}

} // namespace

TEST_CASE( "Read matrix and object inputs in expressions", "[model][inputs]" ) {
  auto model = modelFrom(inputModel(standardInputs), standardContents);
  auto& registry = registryOf(*model);
  BPMNOS::Status status( registry.statusAttributes.size() );
  status.objects.resize( registry.statusObjects.size() );
  BPMNOS::Data data( registry.dataAttributes.size() );
  data.objects.resize( registry.dataObjects.size() );
  auto evaluate = [&](const std::string& text) {
    Model::Expression expression(InputEncoder(text), registry);
    return expression.execute(status, data);
  };

  REQUIRE( evaluate("distance[2][3]") == 7 );
  REQUIRE( evaluate("size(distance)") == 3 );
  REQUIRE( evaluate("sum(flow[3])") == 5 );
  REQUIRE( evaluate("depots[2].x") == 2 );
  REQUIRE( evaluate(R"(depots[1].name == "A")") == 1 );
  REQUIRE( evaluate("depots[2].name") == std::nullopt );
  REQUIRE( to_string(*model->inputs.front()->object) == "[ [ 0, 1, 2 ], [ 1, 0, 3 ], [ 2, 3, 0 ] ]" );

  SECTION( "An input is assigned to an object" ) {
    auto row = registry["row"];
    status.objects[row->index] = row->schema->undefinedObject();
    Model::Expression expression(InputEncoder("row := distance[2]"), registry);
    expression.write(status, data);
    REQUIRE( to_string(*status.objects[row->index]) == "[ 5, 0, 7 ]" );
  }
}

TEST_CASE( "Run the Quadratic Assignment Problem with matrix inputs", "[model][inputs]" ) {
  auto model = modelFrom(inputModel(standardInputs), standardContents);
  auto provider = std::make_shared<Execution::StaticDataProvider>(model, "INSTANCE_ID; NODE_ID; INITIALIZATION\nInstance_1; Process_1;\n");
  Execution::Engine engine(model);
  Execution::InstantEntry entryHandler;
  Execution::InstantExit exitHandler;
  entryHandler.connect(&engine);
  exitHandler.connect(&engine);
  Execution::Recorder recorder;
  recorder.subscribe(&engine);
  engine.run(provider->createScenario());
  auto log = recorder.find(nlohmann::json{{"nodeId","End_1"},{"state","DONE"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
  REQUIRE( log.size() == 1 );
  REQUIRE( log[0]["data"]["cost"] == 70.0 );
}

TEST_CASE( "Fit inputs to their schema", "[model][inputs]" ) {
  SECTION( "A matrix is padded at fixed dimensions" ) {
    auto model = modelWith(R"xml(<bpmnos:input id="Input_1" name="flow" type="matrix" source="flow.csv" schema="decimal[3][3]" />
      <bpmnos:input id="Input_2" name="distance" type="matrix" source="distance.csv" schema="decimal[][]" />)xml", "flow.csv", "1;2\n3;4\n");
    REQUIRE( to_string(*model->inputs.front()->object) == "[ [ 1, 2, undefined ], [ 3, 4, undefined ], [ undefined, undefined, undefined ] ]" );
  }
  SECTION( "An object is padded at fixed dimensions" ) {
    auto model = modelWith(R"xml(<bpmnos:input id="Input_1" name="flow" type="matrix" source="flow.csv" schema="decimal[3][3]" />
      <bpmnos:input id="Input_2" name="distance" type="matrix" source="distance.csv" schema="decimal[][]" />
      <bpmnos:input id="Input_3" name="depots" type="object" source="depots.json" schema="{ x: decimal }[3]" />)xml", "depots.json", R"([ { "x": 1 }, { "x": 2 } ])");
    REQUIRE( to_string(*model->inputs.back()->object) == "[ { x := 1 }, { x := 2 }, { x := undefined } ]" );
  }
  SECTION( "A ragged matrix" ) {
    REQUIRE_THROWS_WITH( modelWith(standardInputs, "distance.csv", "0;5\n5\n"), Catch::Matchers::ContainsSubstring("has 1 cells instead of 2") );
  }
  SECTION( "A cell of the wrong type" ) {
    REQUIRE_THROWS_WITH( modelWith(standardInputs, "distance.csv", "\"A\";5\n5;0\n"), Catch::Matchers::ContainsSubstring("is no decimal") );
  }
  SECTION( "A matrix larger than declared" ) {
    REQUIRE_THROWS_WITH( modelWith(standardInputs, "flow.csv", "0;1;2;3\n1;0;3;4\n2;3;0;5\n"), Catch::Matchers::ContainsSubstring("4 elements where 3 are declared") );
  }
  SECTION( "A schema of a matrix with one dimension" ) {
    REQUIRE_THROWS_WITH( modelFrom(inputModel(R"xml(<bpmnos:input id="Input_1" name="flow" type="matrix" source="flow.csv" schema="decimal[]" />)xml"), standardContents), Catch::Matchers::ContainsSubstring("two dimensions") );
  }
  SECTION( "JSON with an unknown field" ) {
    REQUIRE_THROWS_WITH( modelWith(standardInputs, "depots.json", R"([ { "y": 1 } ])"), Catch::Matchers::ContainsSubstring("unknown field 'y'") );
  }
  SECTION( "JSON with a value of the wrong type" ) {
    REQUIRE_THROWS_WITH( modelWith(standardInputs, "depots.json", R"([ { "x": "far" } ])"), Catch::Matchers::ContainsSubstring("no value of its type") );
  }
  SECTION( "JSON arrays of different lengths at an open dimension" ) {
    REQUIRE_THROWS_WITH( modelFrom(inputModel(R"xml(<bpmnos:input id="Input_1" name="grid" type="object" source="grid.json" schema="decimal[][]" />)xml"), { { "grid.json", "[ [ 1, 2 ], [ 3 ] ]" } }), Catch::Matchers::ContainsSubstring("arrays of different lengths") );
  }
}

TEST_CASE( "Refuse writes to inputs and names used twice", "[model][inputs]" ) {
  auto model = modelFrom(inputModel(standardInputs), standardContents);
  auto& registry = registryOf(*model);
  auto compile = [&registry](const std::string& text) {
    Model::Expression expression(InputEncoder(text), registry);
  };

  REQUIRE_THROWS_WITH( compile("distance[1][1] := 2"), Catch::Matchers::ContainsSubstring("is read only") );
  REQUIRE_THROWS_WITH( compile("distance := [ [ 1 ] ]"), Catch::Matchers::ContainsSubstring("is read only") );
  REQUIRE_THROWS_WITH( compile("resize(distance) := 2"), Catch::Matchers::ContainsSubstring("is read only") );
  REQUIRE_THROWS_WITH( compile("distance + 1"), Catch::Matchers::ContainsSubstring("must be indexed to a value") );
  REQUIRE_THROWS_WITH(
    modelFrom(inputModel(standardInputs, R"xml(<bpmnos:attribute id="Distance" name="distance" type="decimal" />)xml"), standardContents),
    Catch::Matchers::ContainsSubstring("is the name of an input")
  );
  REQUIRE_THROWS_WITH(
    modelFrom(inputModel(standardInputs + R"xml(<bpmnos:input id="Input_4" name="flow" type="matrix" source="flow.csv" schema="decimal[][]" />)xml"), standardContents),
    Catch::Matchers::ContainsSubstring("duplicate input name 'flow'")
  );
}
