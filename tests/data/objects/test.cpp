#include "prelude.h"
#include <map>

namespace {

/// A model with a global object, a process with a status object and a data object, a subprocess with a data
/// object, and a task in it with a status object initialised by the model.
std::shared_ptr<const Model::Model> objectModel() {
  std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
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
          <bpmnos:attribute id="Facilities" name="facilities" type="{ cost: decimal, flags: boolean[2] }[]" />
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
            <bpmnos:attribute id="Grid" name="grid" type="boolean[3][4]" />
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
              <bpmnos:attribute id="Visit" name="visit := { x := 1 }" type="{ x: decimal }" />
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
        <bpmnos:attribute id="Location" name="location" type="integer[3]" />
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
struct ObjectProvider : Execution::StaticDataProvider {
  using StaticDataProvider::StaticDataProvider;
  using StaticDataProvider::getStatus;
  using StaticDataProvider::getData;
};

const BPMN::Node* nodeOf(const Model::Model& model, const std::string& id) {
  auto& process = model.processes.front();
  if ( process->id == id ) {
    return process.get();
  }
  return process->find([&id](BPMN::Node* node) { return node->id == id; });
}

/// Records the number of status objects and data objects of every token at every node and state.
struct ObjectCounter : Execution::Observer {
  std::map<std::pair<std::string, std::string>, std::pair<size_t, size_t>> counts;
  void notice(const Execution::Observable* observable) override {
    auto token = static_cast<const Execution::Token*>(observable);
    auto node = token->node->represents<BPMN::FlowNode>() ? token->node->id : std::string("process");
    counts[{node, Execution::Token::stateName[(int)token->state]}] = { token->status.objects.size(), token->data->objects.size() };
  }
};

const std::string declarations =
  "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
  "; ; location := [3, 1]\n"
  "; Process_1; route[2]\n"
  "; Process_1; facilities[2]\n"
  "Instance_1; Process_1; facilities := [ { cost := 10, flags := [ true ] }, { cost := 20, flags := [ false ] } ]\n"
  "Instance_2; Process_1;\n";

} // namespace

TEST_CASE( "Create objects from the model and the instance data", "[data][objects]" ) {
  auto model = objectModel();
  auto provider = std::make_shared<ObjectProvider>(model, declarations);
  auto scenario = provider->createScenario();
  auto& staticScenario = static_cast<const Execution::StaticDataProvider::Scenario&>(*scenario);
  auto instance1 = (size_t)stringRegistry("Instance_1");
  auto instance2 = (size_t)stringRegistry("Instance_2");

  SECTION( "A global object shorter than declared is padded" ) {
    auto globals = provider->getGlobals(*scenario);
    REQUIRE( globals.objects.size() == 1 );
    REQUIRE( globals.objects[0]->layout->stringify() == "integer[3]" );
    REQUIRE( to_string(*globals.objects[0]) == "[ 3, 1, undefined ]" );
  }

  SECTION( "A status object takes its size from a size declaration" ) {
    auto status = provider->getStatus(staticScenario, instance1, nodeOf(*model, "Process_1"));
    REQUIRE( status.objects.size() == 1 );
    REQUIRE( to_string(*status.objects[0]) == "[ undefined, undefined ]" );
  }

  SECTION( "A data object takes the value an instance gives and is padded at fixed dimensions" ) {
    auto data = provider->getData(staticScenario, instance1, nodeOf(*model, "Process_1"));
    REQUIRE( data.objects.size() == 1 );
    REQUIRE( data.objects[0]->layout->stringify() == "{ cost: decimal, flags: boolean[2] }[2]" );
    REQUIRE( to_string(*data.objects[0]) == "[ { cost := 10, flags := [ true, undefined ] }, { cost := 20, flags := [ false, undefined ] } ]" );
  }

  SECTION( "A data object no instance data gives is undefined in the sizes declared" ) {
    auto data = provider->getData(staticScenario, instance2, nodeOf(*model, "Process_1"));
    REQUIRE( to_string(*data.objects[0]) == "[ { cost := undefined, flags := [ undefined, undefined ] }, { cost := undefined, flags := [ undefined, undefined ] } ]" );
  }

  SECTION( "A data object of a subprocess has the sizes of its type" ) {
    auto data = provider->getData(staticScenario, instance1, nodeOf(*model, "SubProcess_1"));
    REQUIRE( data.objects.size() == 1 );
    REQUIRE( data.objects[0]->layout->stringify() == "boolean[3][4]" );
    REQUIRE( data.objects[0]->values.size() == 12 );
  }

  SECTION( "An object the model initialises is shared by every instance" ) {
    auto status1 = provider->getStatus(staticScenario, instance1, nodeOf(*model, "Task_1"));
    auto status2 = provider->getStatus(staticScenario, instance2, nodeOf(*model, "Task_1"));
    REQUIRE( to_string(*status1.objects[0]) == "{ x := 1 }" );
    REQUIRE( status1.objects[0].get() == status2.objects[0].get() );
  }
}

TEST_CASE( "Create and remove objects during a run", "[data][objects]" ) {
  auto model = objectModel();
  auto provider = std::make_shared<Execution::StaticDataProvider>(model, declarations);
  Execution::Engine engine(model);
  Execution::InstantEntry entryHandler;
  Execution::InstantExit exitHandler;
  entryHandler.connect(&engine);
  exitHandler.connect(&engine);
  ObjectCounter counter;
  engine.addSubscriber(&counter, Execution::Observable::Type::Token);
  engine.run(provider->createScenario());

  // the task holds the status objects of the process and its own, and the data objects of the global state
  // machine, the process and the subprocess
  REQUIRE( counter.counts.at({"Task_1", "BUSY"}) == std::pair<size_t, size_t>{ 2, 3 } );
  // its own status object is removed when it is left
  REQUIRE( counter.counts.at({"End_2", "DONE"}) == std::pair<size_t, size_t>{ 1, 3 } );
  REQUIRE( counter.counts.at({"End_1", "DONE"}) == std::pair<size_t, size_t>{ 1, 2 } );
}

TEST_CASE( "An open dimension without a value has no elements", "[data][objects]" ) {
  auto model = objectModel();
  auto provider = std::make_shared<ObjectProvider>(model, "INSTANCE_ID; NODE_ID; INITIALIZATION\n; Process_1; route[2]\nInstance_1; Process_1;\n");
  auto scenario = provider->createScenario();
  auto& staticScenario = static_cast<const Execution::StaticDataProvider::Scenario&>(*scenario);
  auto data = provider->getData(staticScenario, (size_t)stringRegistry("Instance_1"), nodeOf(*model, "Process_1"));
  REQUIRE( to_string(*data.objects[0]) == "[ ]" );
}

TEST_CASE( "Refuse illegal sizes and values of objects", "[data][objects]" ) {
  auto model = objectModel();
  auto provide = [&model](const std::string& csv) {
    return std::make_shared<Execution::StaticDataProvider>(model, "INSTANCE_ID; NODE_ID; INITIALIZATION\n" + csv);
  };
  const std::string valid = "; Process_1; facilities[2]\n; Process_1; route[2]\nInstance_1; Process_1;\n";

  SECTION( "A value longer than declared" ) {
    REQUIRE_THROWS_WITH( provide("; ; location := [1, 2, 3, 4]\n" + valid), Catch::Matchers::ContainsSubstring("4 elements where 3 are declared") );
  }
  SECTION( "A size contradicting the type" ) {
    REQUIRE_THROWS_WITH( provide("; ; location[4]\n" + valid), Catch::Matchers::ContainsSubstring("contradicts") );
  }
  SECTION( "Size declarations contradicting each other" ) {
    REQUIRE_THROWS_WITH( provide("; Process_1; route[3]\n" + valid), Catch::Matchers::ContainsSubstring("contradicts") );
  }
  SECTION( "A size declaration for a node not declaring the object" ) {
    REQUIRE_THROWS_WITH( provide("; SubProcess_1; route[2]\n" + valid), Catch::Matchers::ContainsSubstring("names no object") );
  }
  SECTION( "A value for an object the model initialises" ) {
    REQUIRE_THROWS_WITH( provide(valid + "Instance_1; Task_1; visit := { x := 2 }\n"), Catch::Matchers::ContainsSubstring("assigned by the model") );
  }
  SECTION( "A ragged literal" ) {
    REQUIRE_THROWS( provide(valid + "Instance_1; Process_1; facilities := [ { cost := 1, flags := [ true ] }, { cost := 2, flags := [ true, false ] } ]\n") );
  }
  SECTION( "A disclosure of an object" ) {
    REQUIRE_THROWS_WITH(
      std::make_shared<Execution::DynamicDataProvider>(model,
        "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
        "; Process_1; facilities[2];\n"
        "; Process_1; route[2];\n"
        "Instance_1; Process_1; facilities := [ { cost := 1 } ]; 3\n"
      ),
      Catch::Matchers::ContainsSubstring("known from the start")
    );
  }
}
