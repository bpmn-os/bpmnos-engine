#include "prelude.h"

namespace {

/// The deferrals inserted into the test model, each empty if the model states none there.
struct Deferrals {
  std::string performer;
  std::string choice;
  std::string message;
  std::string entry;
};

/// Returns a model whose process declares the sequential performer of its ad-hoc subprocess, followed by a decision
/// task and a receive task, with the given deferrals. The data attribute `count` is written by an operator, the data
/// attribute `fixed` by none.
std::unique_ptr<Model::Model> modelWith(const Deferrals& deferrals) {
  std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:dataStore id="DataStore_Globals" name="Globals">
    <bpmn2:extensionElements>
      <bpmnos:attributes>
        <bpmnos:attribute id="Time" type="decimal" name="time" />
      </bpmnos:attributes>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:performer name="Sequential">
      <bpmn2:extensionElements>)" + deferrals.performer + R"(
      </bpmn2:extensionElements>
    </bpmn2:performer>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" name="instance" type="string" />
          <bpmnos:attribute id="Count" name="count := 0" type="integer" />
          <bpmnos:attribute id="Fixed" name="fixed := 1" type="integer" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="AdHocSubProcess_1" />
    <bpmn2:adHocSubProcess id="AdHocSubProcess_1" ordering="Sequential">
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_2</bpmn2:outgoing>
      <bpmn2:task id="Activity_1">
        <bpmn2:extensionElements>
          <bpmnos:status>
            <bpmnos:operators>
              <bpmnos:operator id="Operator_1" expression="count += 1" />
            </bpmnos:operators>
          </bpmnos:status>
          <bpmnos:guidance type="entry">)" + deferrals.entry + R"(
          </bpmnos:guidance>
        </bpmn2:extensionElements>
      </bpmn2:task>
    </bpmn2:adHocSubProcess>
    <bpmn2:sequenceFlow id="Flow_2" sourceRef="AdHocSubProcess_1" targetRef="Choice_1" />
    <bpmn2:task id="Choice_1" bpmnos:type="Decision">
      <bpmn2:extensionElements>
        <bpmnos:status>
          <bpmnos:attributes>
            <bpmnos:attribute id="Choice" name="choice" type="integer" />
          </bpmnos:attributes>
          <bpmnos:decisions>
            <bpmnos:decision id="Decision_1" condition="1 &lt;= choice &lt;= 3" />
          </bpmnos:decisions>
        </bpmnos:status>
        <bpmnos:guidance type="choice">)" + deferrals.choice + R"(
        </bpmnos:guidance>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_2</bpmn2:incoming>
      <bpmn2:outgoing>Flow_3</bpmn2:outgoing>
    </bpmn2:task>
    <bpmn2:sequenceFlow id="Flow_3" sourceRef="Choice_1" targetRef="Receive_1" />
    <bpmn2:receiveTask id="Receive_1">
      <bpmn2:extensionElements>
        <bpmnos:message name="Message" />
        <bpmnos:guidance type="message">)" + deferrals.message + R"(
        </bpmnos:guidance>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_3</bpmn2:incoming>
      <bpmn2:outgoing>Flow_4</bpmn2:outgoing>
    </bpmn2:receiveTask>
    <bpmn2:sequenceFlow id="Flow_4" sourceRef="Receive_1" targetRef="End_1" />
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
  return std::make_unique<Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

const Model::ExtensionElements* extensionElementsOf(const Model::Model& model, const std::string& id) {
  auto& process = model.processes.front();
  auto node = process->find([&id](const BPMN::Node* candidate) { return candidate->id == id; });
  REQUIRE( node );
  return node->extensionElements->as<Model::ExtensionElements>();
}

} // namespace

TEST_CASE( "Deferrals are read at a performer, in a choice guidance and in a message guidance", "[model][deferral]" ) {
  auto model = modelWith({
    .performer = R"(<bpmnos:deferral condition="count &lt; 3" />)",
    .choice = R"(<bpmnos:deferral condition="time &lt; 10"><bpmnos:parameter name="observable" value="count" /><bpmnos:parameter name="observable" value="time" /></bpmnos:deferral>)",
    .message = R"(<bpmnos:deferral condition="fixed &gt; 0" />)",
    .entry = ""
  });

  auto processExtension = extensionElementsOf(*model, "Process_1");
  auto& registry = processExtension->attributeRegistry;

  SECTION( "The deferral of the performer is attached to the node declaring the performer" ) {
    REQUIRE( processExtension->hasSequentialPerformer );
    auto& deferral = processExtension->performerDeferral;
    REQUIRE( deferral );
    REQUIRE( deferral->observables.empty() );
    REQUIRE( deferral->condition.inputs.contains(registry["count"]) );

    BPMNOS::Status status(registry.statusAttributes.size());
    BPMNOS::Data data(registry.dataAttributes.size());
    data.attributes[registry["count"]->index] = BPMNOS::number(1);
    REQUIRE( deferral->conditionHolds(status, data) );
    data.attributes[registry["count"]->index] = BPMNOS::number(5);
    REQUIRE( !deferral->conditionHolds(status, data) );
  }

  SECTION( "The deferral of a choice guidance names its observed attributes in the order stated" ) {
    auto& guidance = extensionElementsOf(*model, "Choice_1")->choiceGuidance;
    REQUIRE( guidance.has_value() );
    auto& deferral = guidance.value()->deferral;
    REQUIRE( deferral );
    REQUIRE( deferral->observables == std::vector<const Model::Attribute*>{ registry["count"], registry["time"] } );
  }

  SECTION( "The deferral of a message guidance is read" ) {
    auto& guidance = extensionElementsOf(*model, "Receive_1")->messageDeliveryGuidance;
    REQUIRE( guidance.has_value() );
    REQUIRE( guidance.value()->deferral );
  }
}

TEST_CASE( "A model without deferrals permits none", "[model][deferral]" ) {
  auto model = modelWith({});
  REQUIRE( !extensionElementsOf(*model, "Process_1")->performerDeferral );
  REQUIRE( !extensionElementsOf(*model, "Choice_1")->choiceGuidance.value()->deferral );
  REQUIRE( !extensionElementsOf(*model, "Receive_1")->messageDeliveryGuidance.value()->deferral );
}

TEST_CASE( "Illegal deferrals are refused", "[model][deferral]" ) {
  SECTION( "Two deferrals in one guidance" ) {
    REQUIRE_THROWS( modelWith({ .performer = "", .choice = R"(<bpmnos:deferral condition="true" /><bpmnos:deferral condition="true" />)", .message = "", .entry = "" }) );
  }
  SECTION( "Two deferrals at one performer" ) {
    REQUIRE_THROWS( modelWith({ .performer = R"(<bpmnos:deferral condition="true" /><bpmnos:deferral condition="true" />)", .choice = "", .message = "", .entry = "" }) );
  }
  SECTION( "A deferral in an entry guidance" ) {
    REQUIRE_THROWS( modelWith({ .performer = "", .choice = "", .message = "", .entry = R"(<bpmnos:deferral condition="true" />)" }) );
  }
  SECTION( "An observed attribute not in scope" ) {
    REQUIRE_THROWS( modelWith({ .performer = "", .choice = R"(<bpmnos:deferral condition="true"><bpmnos:parameter name="observable" value="unknown" /></bpmnos:deferral>)", .message = "", .entry = "" }) );
  }
  SECTION( "An observed status attribute" ) {
    REQUIRE_THROWS( modelWith({ .performer = "", .choice = R"(<bpmnos:deferral condition="true"><bpmnos:parameter name="observable" value="timestamp" /></bpmnos:deferral>)", .message = "", .entry = "" }) );
  }
  SECTION( "An observed attribute that is never updated" ) {
    REQUIRE_THROWS( modelWith({ .performer = "", .choice = R"(<bpmnos:deferral condition="true"><bpmnos:parameter name="observable" value="fixed" /></bpmnos:deferral>)", .message = "", .entry = "" }) );
  }
  SECTION( "A parameter other than an observable" ) {
    REQUIRE_THROWS( modelWith({ .performer = "", .choice = "", .message = R"(<bpmnos:deferral condition="true"><bpmnos:parameter name="trigger" value="count" /></bpmnos:deferral>)", .entry = "" }) );
  }
  SECTION( "An assignment as condition" ) {
    REQUIRE_THROWS( modelWith({ .performer = R"(<bpmnos:deferral condition="count := 1" />)", .choice = "", .message = "", .entry = "" }) );
  }
}
