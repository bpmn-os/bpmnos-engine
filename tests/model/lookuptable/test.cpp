#include "prelude.h"
#include <map>

TEST_CASE( "Check every cell of a lookup table against the type of its column", "[model][lookuptable]" ) {
  const std::string content =
    "Location; Destinations\n"
    "\"A\"; [ \"X\", \"Y\" ]\n"
    "\"B\"; [ \"Z\" ]\n";

  SECTION( "A lookup returning an array of different lengths" ) {
    Model::LookupTable table("destinations", content, "Location: string;Destinations: string[]");
    REQUIRE( to_string(*objectRegistry[(size_t)table.at({ (double)stringRegistry("A") })]) == R"([ "X", "Y" ])" );
    REQUIRE( to_string(*objectRegistry[(size_t)table.at({ (double)stringRegistry("B") })]) == R"([ "Z" ])" );
  }
  SECTION( "A column without a type" ) {
    REQUIRE_THROWS_WITH( Model::LookupTable("destinations", content, "Location;Destinations"), Catch::Matchers::ContainsSubstring("requires a type") );
  }
  SECTION( "A key column of an array type" ) {
    REQUIRE_THROWS_WITH( Model::LookupTable("destinations", content, "Location: string[];Destinations: string[]"), Catch::Matchers::ContainsSubstring("must have a scalar type") );
  }
  SECTION( "A literal where a number is declared" ) {
    REQUIRE_THROWS_WITH( Model::LookupTable("destinations", content, "Location: string;Destinations: decimal"), Catch::Matchers::ContainsSubstring("is no decimal") );
  }
  SECTION( "A string where a number is declared" ) {
    REQUIRE_THROWS_WITH( Model::LookupTable("destinations", content, "Location: decimal;Destinations: string[]"), Catch::Matchers::ContainsSubstring("is no decimal") );
  }
  SECTION( "An array not fitting its type" ) {
    REQUIRE_THROWS_WITH( Model::LookupTable("destinations", content, "Location: string;Destinations: decimal[]"), Catch::Matchers::ContainsSubstring("is no decimal[]") );
  }
  SECTION( "A column name not agreeing with the file" ) {
    REQUIRE_THROWS_WITH( Model::LookupTable("destinations", content, "Origin: string;Destinations: string[]"), Catch::Matchers::ContainsSubstring("expects column 1") );
  }
}

namespace {

/// A model whose process initialises an array by a lookup and whose task assigns it another lookup result.
std::shared_ptr<const Model::Model> lookupModel() {
  std::string xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
          <bpmnos:attribute id="Location" name="location" type="string" />
          <bpmnos:attribute id="Destinations" name="destinations := destinations(location)" type="string[]" />
          <bpmnos:attribute id="Length" name="length" type="integer" />
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
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="Task_1" />
    <bpmn2:task id="Task_1">
      <bpmn2:extensionElements>
        <bpmnos:status>
          <bpmnos:operators>
            <bpmnos:operator id="Operator_1" expression="destinations := destinations(&quot;B&quot;)" />
            <bpmnos:operator id="Operator_2" expression="length := size(destinations)" />
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
      <bpmnos:tables>
        <bpmnos:table id="Table_1" name="destinations" source="destinations.csv" header="Location: string;Destinations: string[]" />
      </bpmnos:tables>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
</bpmn2:definitions>
)xml";
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{
    { "destinations.csv", "Location; Destinations\n\"A\"; [ \"X\", \"Y\" ]\n\"B\"; [ \"Z\" ]\n" }
  });
}

/// Records a copy of the status of every token at every node and state.
struct StatusCapture : Execution::Observer {
  std::map<std::pair<std::string, std::string>, BPMNOS::Status> statuses;
  void notice(const Execution::Observable* observable) override {
    auto token = static_cast<const Execution::Token*>(observable);
    auto node = token->node->represents<BPMN::FlowNode>() ? token->node->id : std::string("process");
    statuses[{node, Execution::Token::stateName[(int)token->state]}] = token->status;
  }
};

} // namespace

TEST_CASE( "Assign arrays a lookup returns", "[model][lookuptable]" ) {
  auto model = lookupModel();
  auto& registry = model->processes.front()->extensionElements->as<Model::ExtensionElements>()->attributeRegistry;

  SECTION( "The initial value and an operator assign arrays of different lengths" ) {
    auto provider = std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; location := \"A\"\n"
    );
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    StatusCapture capture;
    engine.addSubscriber(&capture, Execution::Observable::Type::Token);
    engine.run(provider->createScenario());

    auto destinations = registry["destinations"];
    REQUIRE( to_string(*capture.statuses.at({"Task_1", "READY"}).objects[destinations->index]) == R"([ "X", "Y" ])" );
    auto& done = capture.statuses.at({"End_1", "DONE"});
    REQUIRE( to_string(*done.objects[destinations->index]) == R"([ "Z" ])" );
    REQUIRE( done.attributes[registry["length"]->index] == 1 );
  }

  SECTION( "A lookup returning an array is only assigned to an array" ) {
    auto compile = [&registry](const std::string& text) {
      Model::Expression expression(InputEncoder(text), registry);
    };
    REQUIRE_THROWS_WITH( compile(R"(length := destinations("A"))"), Catch::Matchers::ContainsSubstring("returns an array") );
    REQUIRE_THROWS_WITH( compile(R"(destinations("A") == 1)"), Catch::Matchers::ContainsSubstring("returns an array") );
    REQUIRE_NOTHROW( compile(R"(destinations := destinations(location))") );
  }
}
