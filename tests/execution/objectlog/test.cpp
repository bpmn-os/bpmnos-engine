#include "prelude.h"

namespace {

/// A model whose status, data and globals each declare scalar attributes and objects, interleaved.
std::shared_ptr<const Model::Model> logModel() {
  std::string xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
          <bpmnos:attribute id="Route" name="route" type="integer[]" />
          <bpmnos:attribute id="Count" name="count" type="integer" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Facilities" name="facilities" type="{ cost: decimal, open: boolean, name: string, flags: boolean[2] }[]" />
          <bpmnos:attribute id="Instance" name="instance" type="string" />
          <bpmnos:attribute id="Label" name="label" type="string" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="End_1" />
    <bpmn2:endEvent id="End_1">
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
  <bpmn2:dataStore id="DataStore_1">
    <bpmn2:extensionElements>
      <bpmnos:attributes>
        <bpmnos:attribute id="Location" name="location" type="integer[3]" />
        <bpmnos:attribute id="Limit" name="limit" type="integer" />
      </bpmnos:attributes>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
</bpmn2:definitions>
)xml";
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

std::vector<std::string> keysOf(const nlohmann::ordered_json& object) {
  std::vector<std::string> keys;
  for ( auto& [key, _] : object.items() ) {
    keys.push_back(key);
  }
  return keys;
}

} // namespace

TEST_CASE( "The log shows objects as JSON in the order of declaration", "[execution][objects][log]" ) {
  auto model = logModel();
  auto provider = std::make_shared<Execution::StaticDataProvider>(model,
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "; ; location := [3]\n"
    "; ; limit := 3\n"
    "Instance_1; Process_1; route := [1, 2]\n"
    "Instance_1; Process_1; count := 2\n"
    "Instance_1; Process_1; facilities := [ { cost := 10, open := true, name := \"North\", flags := [ true ] } ]\n"
    "Instance_1; Process_1; label := \"Depot\"\n"
  );
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
  auto& entry = log[0];

  SECTION( "Objects are rendered as JSON" ) {
    REQUIRE( entry["status"]["route"] == nlohmann::ordered_json::array({ 1, 2 }) );
    REQUIRE( entry["data"]["facilities"] == nlohmann::ordered_json::parse(R"([ { "cost": 10.0, "open": true, "name": "North", "flags": [ true, null ] } ])") );
    REQUIRE( entry["globals"]["location"] == nlohmann::ordered_json::parse("[ 3, null, null ]") );
  }

  SECTION( "Attributes and objects appear in the order of their declaration" ) {
    REQUIRE( keysOf(entry["status"]) == std::vector<std::string>{ "timestamp", "route", "count" } );
    // the instance is always the first data attribute of a process
    REQUIRE( keysOf(entry["data"]) == std::vector<std::string>{ "instance", "facilities", "label" } );
    REQUIRE( keysOf(entry["globals"]) == std::vector<std::string>{ "location", "limit" } );
  }
}
