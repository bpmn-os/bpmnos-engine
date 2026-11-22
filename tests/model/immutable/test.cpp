#include "prelude.h"

namespace {

/// What is inserted into the test model, each piece empty or as given by default if the model states nothing there.
struct Insertions {
  std::string timeDeclaration = R"(immutable="false")";
  std::string timestampDeclaration = "";
  std::string processStart = "";
  std::string task = "";
  std::string subProcessStart = "";
  std::string eventSubProcessStart = "";
  std::string decision = R"(1 &lt;= pick &lt;= 3)";
  std::string choiceGuidance = "";
  std::string iterationDeclaration = "";
};

std::string operators(const std::string& expressions) {
  return expressions.empty() ? "" : "<bpmnos:operators>" + expressions + "</bpmnos:operators>";
}

/// Returns a model with an immutable global `limit` and a changeable global `level`; a process with the immutable
/// status `label` and the data `fixed`, immutable, and `count`, changeable; a task; a subprocess with the immutable
/// data `inner`; a non-interrupting event subprocess with the immutable data `token`; a decision task choosing `pick`;
/// and a task looping with the index `iteration`. Each start event, the task and the choice guidance carry the
/// operators given.
std::unique_ptr<Model::Model> modelWith(const Insertions& insertions) {
  std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:dataStore id="DataStore_Globals" name="Globals">
    <bpmn2:extensionElements>
      <bpmnos:attributes>
        <bpmnos:attribute id="Time" type="decimal" name="time" )" + insertions.timeDeclaration + R"( />
        <bpmnos:attribute id="Limit" type="integer" name="limit := 10" immutable="true" />
        <bpmnos:attribute id="Level" type="integer" name="level := 0" />
      </bpmnos:attributes>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" )" + insertions.timestampDeclaration + R"( />
          <bpmnos:attribute id="Label" name="label := 0" type="integer" immutable="true" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" name="instance" type="string" />
          <bpmnos:attribute id="Fixed" name="fixed := 1" type="integer" immutable="true" />
          <bpmnos:attribute id="Count" name="count := 0" type="integer" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:extensionElements>
        <bpmnos:status>)" + operators(insertions.processStart) + R"(</bpmnos:status>
      </bpmn2:extensionElements>
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="Task_1" />
    <bpmn2:task id="Task_1">
      <bpmn2:extensionElements>
        <bpmnos:status>)" + operators(insertions.task) + R"(</bpmnos:status>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_2</bpmn2:outgoing>
    </bpmn2:task>
    <bpmn2:sequenceFlow id="Flow_2" sourceRef="Task_1" targetRef="SubProcess_1" />
    <bpmn2:subProcess id="SubProcess_1">
      <bpmn2:incoming>Flow_2</bpmn2:incoming>
      <bpmn2:outgoing>Flow_3</bpmn2:outgoing>
      <bpmn2:dataObject id="DataObject_2">
        <bpmn2:extensionElements>
          <bpmnos:attributes>
            <bpmnos:attribute id="Inner" name="inner := 1" type="integer" immutable="true" />
          </bpmnos:attributes>
        </bpmn2:extensionElements>
      </bpmn2:dataObject>
      <bpmn2:startEvent id="SubStart_1">
        <bpmn2:extensionElements>
          <bpmnos:status>)" + operators(insertions.subProcessStart) + R"(</bpmnos:status>
        </bpmn2:extensionElements>
        <bpmn2:outgoing>Flow_Sub</bpmn2:outgoing>
      </bpmn2:startEvent>
      <bpmn2:sequenceFlow id="Flow_Sub" sourceRef="SubStart_1" targetRef="SubEnd_1" />
      <bpmn2:endEvent id="SubEnd_1">
        <bpmn2:incoming>Flow_Sub</bpmn2:incoming>
      </bpmn2:endEvent>
    </bpmn2:subProcess>
    <bpmn2:sequenceFlow id="Flow_3" sourceRef="SubProcess_1" targetRef="Choice_1" />
    <bpmn2:task id="Choice_1" bpmnos:type="Decision">
      <bpmn2:extensionElements>
        <bpmnos:status>
          <bpmnos:attributes>
            <bpmnos:attribute id="Pick" name="pick" type="integer" />
          </bpmnos:attributes>
          <bpmnos:decisions>
            <bpmnos:decision id="Decision_1" condition=")" + insertions.decision + R"(" />
          </bpmnos:decisions>
        </bpmnos:status>
        <bpmnos:guidance type="choice">
          <bpmnos:attributes>
            <bpmnos:attribute id="Score" name="score" type="decimal" objective="minimize" weight="1" />
          </bpmnos:attributes>)" + operators(insertions.choiceGuidance) + R"(
        </bpmnos:guidance>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_3</bpmn2:incoming>
      <bpmn2:outgoing>Flow_4</bpmn2:outgoing>
    </bpmn2:task>
    <bpmn2:sequenceFlow id="Flow_4" sourceRef="Choice_1" targetRef="Loop_1" />
    <bpmn2:task id="Loop_1">
      <bpmn2:extensionElements>
        <bpmnos:status>
          <bpmnos:attributes>
            <bpmnos:attribute id="Iteration" name="iteration" type="integer" )" + insertions.iterationDeclaration + R"( />
          </bpmnos:attributes>
        </bpmnos:status>
        <bpmnos:loopCharacteristics>
          <bpmnos:parameter name="index" value="iteration" />
          <bpmnos:parameter name="maximum" value="2" />
        </bpmnos:loopCharacteristics>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_4</bpmn2:incoming>
      <bpmn2:outgoing>Flow_5</bpmn2:outgoing>
      <bpmn2:standardLoopCharacteristics />
    </bpmn2:task>
    <bpmn2:sequenceFlow id="Flow_5" sourceRef="Loop_1" targetRef="End_1" />
    <bpmn2:endEvent id="End_1">
      <bpmn2:incoming>Flow_5</bpmn2:incoming>
    </bpmn2:endEvent>
    <bpmn2:subProcess id="EventSubProcess_1" triggeredByEvent="true">
      <bpmn2:dataObject id="DataObject_3">
        <bpmn2:extensionElements>
          <bpmnos:attributes>
            <bpmnos:attribute id="Token" name="token := 1" type="integer" immutable="true" />
          </bpmnos:attributes>
        </bpmn2:extensionElements>
      </bpmn2:dataObject>
      <bpmn2:startEvent id="EventSubStart_1" isInterrupting="false">
        <bpmn2:extensionElements>
          <bpmnos:status>)" + operators(insertions.eventSubProcessStart) + R"(</bpmnos:status>
          <bpmnos:signal name="Ping" />
        </bpmn2:extensionElements>
        <bpmn2:outgoing>Flow_Event</bpmn2:outgoing>
        <bpmn2:signalEventDefinition id="SignalEventDefinition_1" />
      </bpmn2:startEvent>
      <bpmn2:sequenceFlow id="Flow_Event" sourceRef="EventSubStart_1" targetRef="EventSubEnd_1" />
      <bpmn2:endEvent id="EventSubEnd_1">
        <bpmn2:incoming>Flow_Event</bpmn2:incoming>
      </bpmn2:endEvent>
    </bpmn2:subProcess>
  </bpmn2:process>
</bpmn2:definitions>
)";
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_unique<Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

std::string operatorWith(const std::string& expression) {
  return R"(<bpmnos:operator id="Operator_1" expression=")" + expression + R"(" />)";
}

const Model::Attribute* attributeOf(const Model::Model& model, const std::string& name) {
  auto& process = model.processes.front();
  for ( auto node : process->find_all([](const BPMN::Node* candidate) { return candidate->extensionElements != nullptr; }) ) {
    auto& registry = node->extensionElements->as<Model::ExtensionElements>()->attributeRegistry;
    if ( registry.contains(name) ) {
      return registry[name];
    }
  }
  FAIL( "no attribute '" + name + "'" );
  return nullptr;
}

} // namespace

TEST_CASE( "An attribute is immutable if and only if it is declared so, the instance always", "[model][immutable]" ) {
  auto model = modelWith({});
  for ( auto name : { "limit", "label", "fixed", "inner", "token", "instance" } ) {
    INFO( name );
    REQUIRE( attributeOf(*model, name)->isImmutable );
  }
  // an attribute that no operator writes is changeable unless it is declared immutable, since the environment may
  // write it
  for ( auto name : { "time", "timestamp", "level", "count", "pick", "iteration" } ) {
    INFO( name );
    REQUIRE( !attributeOf(*model, name)->isImmutable );
  }
}

TEST_CASE( "The start event of the scope declaring an immutable attribute may set it", "[model][immutable]" ) {
  REQUIRE_NOTHROW( modelWith({ .processStart = operatorWith("fixed := 2") + R"(<bpmnos:operator id="Operator_2" expression="label := 3" />)" }) );
  REQUIRE_NOTHROW( modelWith({ .subProcessStart = operatorWith("inner := 2") }) );
  REQUIRE_NOTHROW( modelWith({ .eventSubProcessStart = operatorWith("token := 2") }) );
  REQUIRE_NOTHROW( modelWith({ .task = operatorWith("count := 2") }) );
}

TEST_CASE( "Nothing else may assign an immutable attribute", "[model][immutable]" ) {
  SECTION( "An operator of a task" ) {
    REQUIRE_THROWS_WITH( modelWith({ .task = operatorWith("fixed := 2") }),
      Catch::Matchers::ContainsSubstring("operator 'Operator_1' of 'Task_1' assigns immutable attribute 'Fixed'") );
  }
  SECTION( "An operator of the start event of a subprocess assigning an attribute of the enclosing process" ) {
    REQUIRE_THROWS_WITH( modelWith({ .subProcessStart = operatorWith("fixed := 2") }),
      Catch::Matchers::ContainsSubstring("operator 'Operator_1' of 'SubStart_1' assigns immutable attribute 'Fixed'") );
  }
  SECTION( "An operator of the start event of a process assigning a global attribute" ) {
    REQUIRE_THROWS_WITH( modelWith({ .processStart = operatorWith("limit := 2") }),
      Catch::Matchers::ContainsSubstring("operator 'Operator_1' of 'Start_1' assigns immutable attribute 'Limit'") );
  }
  SECTION( "An operator of the start event of an event subprocess assigning an attribute of the process" ) {
    REQUIRE_THROWS_WITH( modelWith({ .eventSubProcessStart = operatorWith("fixed := 2") }),
      Catch::Matchers::ContainsSubstring("operator 'Operator_1' of 'EventSubStart_1' assigns immutable attribute 'Fixed'") );
  }
  SECTION( "A choice" ) {
    REQUIRE_THROWS_WITH( modelWith({ .decision = "1 &lt;= fixed &lt;= 3" }),
      Catch::Matchers::ContainsSubstring("Choice: attribute 'Fixed' is immutable and must not be chosen") );
  }
  SECTION( "An operator of a guidance" ) {
    REQUIRE_THROWS_WITH( modelWith({ .choiceGuidance = operatorWith("fixed := 2") }),
      Catch::Matchers::ContainsSubstring("Operator: attribute 'Fixed' is immutable and must not be assigned") );
  }
  SECTION( "The index of a standard loop, which the engine increments" ) {
    REQUIRE_THROWS_WITH( modelWith({ .iterationDeclaration = R"(immutable="true")" }),
      Catch::Matchers::ContainsSubstring("immutable attribute 'Iteration' must not be the loop index of standard loop activity 'Loop_1'") );
  }
}

TEST_CASE( "The time and the timestamp must not be declared immutable", "[model][immutable]" ) {
  REQUIRE_THROWS_WITH( modelWith({ .timeDeclaration = R"(immutable="true")" }),
    Catch::Matchers::ContainsSubstring("Attribute: 'Time' must not be immutable") );
  REQUIRE_THROWS_WITH( modelWith({ .timestampDeclaration = R"(immutable="true")" }),
    Catch::Matchers::ContainsSubstring("Attribute: 'Timestamp' must not be immutable") );
}
