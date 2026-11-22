#include "prelude.h"

namespace {

/// Returns a model with the global attribute `flag`, and a process with the data `level`, weighted by two in the
/// objective, and the immutable data `fixed`. The process forks into three branches: a conditional event waiting for
/// `flag`, a conditional event waiting for `level >= 3`, and a subprocess with the data `inner`, whose conditional event
/// waits for `inner >= 1`.
std::shared_ptr<const Model::Model> model() {
  std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:dataStore id="DataStore_Globals" name="Globals">
    <bpmn2:extensionElements>
      <bpmnos:attributes>
        <bpmnos:attribute id="Time" type="decimal" name="time" />
        <bpmnos:attribute id="Flag" type="boolean" name="flag := false" />
      </bpmnos:attributes>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" type="decimal" name="timestamp" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" type="string" name="instance" />
          <bpmnos:attribute id="Level" type="integer" name="level := 0" objective="maximize" weight="2" />
          <bpmnos:attribute id="Fixed" type="integer" name="fixed := 1" immutable="true" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="Fork_1" />
    <bpmn2:parallelGateway id="Fork_1">
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_Global</bpmn2:outgoing>
      <bpmn2:outgoing>Flow_Data</bpmn2:outgoing>
      <bpmn2:outgoing>Flow_Sub</bpmn2:outgoing>
    </bpmn2:parallelGateway>
    <bpmn2:sequenceFlow id="Flow_Global" sourceRef="Fork_1" targetRef="GlobalCondition_1" />
    <bpmn2:intermediateCatchEvent id="GlobalCondition_1">
      <bpmn2:extensionElements>
        <bpmnos:restrictions>
          <bpmnos:restriction id="Condition_Global" expression="flag == true" />
        </bpmnos:restrictions>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_Global</bpmn2:incoming>
      <bpmn2:outgoing>Flow_Global_End</bpmn2:outgoing>
      <bpmn2:conditionalEventDefinition id="Definition_Global">
        <bpmn2:condition xsi:type="bpmn2:tFormalExpression" />
      </bpmn2:conditionalEventDefinition>
    </bpmn2:intermediateCatchEvent>
    <bpmn2:sequenceFlow id="Flow_Global_End" sourceRef="GlobalCondition_1" targetRef="GlobalEnd_1" />
    <bpmn2:endEvent id="GlobalEnd_1">
      <bpmn2:incoming>Flow_Global_End</bpmn2:incoming>
    </bpmn2:endEvent>
    <bpmn2:sequenceFlow id="Flow_Data" sourceRef="Fork_1" targetRef="DataCondition_1" />
    <bpmn2:intermediateCatchEvent id="DataCondition_1">
      <bpmn2:extensionElements>
        <bpmnos:restrictions>
          <bpmnos:restriction id="Condition_Data" expression="level &gt;= 3" />
        </bpmnos:restrictions>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_Data</bpmn2:incoming>
      <bpmn2:outgoing>Flow_Data_End</bpmn2:outgoing>
      <bpmn2:conditionalEventDefinition id="Definition_Data">
        <bpmn2:condition xsi:type="bpmn2:tFormalExpression" />
      </bpmn2:conditionalEventDefinition>
    </bpmn2:intermediateCatchEvent>
    <bpmn2:sequenceFlow id="Flow_Data_End" sourceRef="DataCondition_1" targetRef="DataEnd_1" />
    <bpmn2:endEvent id="DataEnd_1">
      <bpmn2:incoming>Flow_Data_End</bpmn2:incoming>
    </bpmn2:endEvent>
    <bpmn2:sequenceFlow id="Flow_Sub" sourceRef="Fork_1" targetRef="SubProcess_1" />
    <bpmn2:subProcess id="SubProcess_1">
      <bpmn2:incoming>Flow_Sub</bpmn2:incoming>
      <bpmn2:outgoing>Flow_Sub_End</bpmn2:outgoing>
      <bpmn2:dataObject id="DataObject_2">
        <bpmn2:extensionElements>
          <bpmnos:attributes>
            <bpmnos:attribute id="Inner" type="integer" name="inner := 0" />
          </bpmnos:attributes>
        </bpmn2:extensionElements>
      </bpmn2:dataObject>
      <bpmn2:startEvent id="SubStart_1">
        <bpmn2:outgoing>Flow_Inner</bpmn2:outgoing>
      </bpmn2:startEvent>
      <bpmn2:sequenceFlow id="Flow_Inner" sourceRef="SubStart_1" targetRef="InnerCondition_1" />
      <bpmn2:intermediateCatchEvent id="InnerCondition_1">
        <bpmn2:extensionElements>
          <bpmnos:restrictions>
            <bpmnos:restriction id="Condition_Inner" expression="inner &gt;= 1" />
          </bpmnos:restrictions>
        </bpmn2:extensionElements>
        <bpmn2:incoming>Flow_Inner</bpmn2:incoming>
        <bpmn2:outgoing>Flow_Inner_End</bpmn2:outgoing>
        <bpmn2:conditionalEventDefinition id="Definition_Inner">
          <bpmn2:condition xsi:type="bpmn2:tFormalExpression" />
        </bpmn2:conditionalEventDefinition>
      </bpmn2:intermediateCatchEvent>
      <bpmn2:sequenceFlow id="Flow_Inner_End" sourceRef="InnerCondition_1" targetRef="SubEnd_1" />
      <bpmn2:endEvent id="SubEnd_1">
        <bpmn2:incoming>Flow_Inner_End</bpmn2:incoming>
      </bpmn2:endEvent>
    </bpmn2:subProcess>
    <bpmn2:sequenceFlow id="Flow_Sub_End" sourceRef="SubProcess_1" targetRef="SubProcessEnd_1" />
    <bpmn2:endEvent id="SubProcessEnd_1">
      <bpmn2:incoming>Flow_Sub_End</bpmn2:incoming>
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

/// A static data provider that enqueues the given events once the run reaches the time given with each.
class UpdatingDataProvider : public Execution::StaticDataProvider {
public:
  using Producer = std::function<std::shared_ptr<Execution::Event>(const Execution::SystemState*)>;

  UpdatingDataProvider(std::shared_ptr<const Model::Model> model, const std::string& csv, std::vector<std::pair<double, Producer>> updates)
    : StaticDataProvider(model, csv)
    , updates(std::move(updates))
  {
  }

  void dispatchEvent(const Execution::SystemState* systemState, Execution::Scenario& scenario, Execution::EventQueue& queue) const override {
    StaticDataProvider::dispatchEvent(systemState, scenario, queue);
    while ( next < updates.size() && BPMNOS::number(updates[next].first) <= systemState->getTime() ) {
      queue.push_back(updates[next].second(systemState));
      next++;
    }
  }

private:
  std::vector<std::pair<double, Producer>> updates;
  mutable size_t next = 0;
};

const std::string csv =
  "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
  "Instance_1; Process_1;\n"
;

const BPMN::Scope* scopeOf(const Model::Model& model, const std::string& id) {
  auto& process = model.processes.front();
  if ( process->id == id ) {
    return process.get();
  }
  auto node = process->find([&id](const BPMN::Node* candidate) { return candidate->id == id; });
  REQUIRE( node );
  return node->represents<BPMN::Scope>();
}

const Model::Attribute* attributeOf(const Model::Model& model, const std::string& scopeId, const std::string& name) {
  return scopeOf(model, scopeId)->extensionElements->as<Model::ExtensionElements>()->attributeRegistry[name];
}

/// Runs the model until the end time 5 with the given updates and keeps the recorder of the run and the objective it
/// ends with. The engine is declared before the observers subscribed to it, so that they are destroyed first.
struct Run {
  std::shared_ptr<const Model::Model> model;
  std::shared_ptr<UpdatingDataProvider> dataProvider;
  Execution::Engine engine;
  Execution::InstantEntry entryHandler;
  Execution::Recorder recorder;
  BPMNOS::number objective = 0;

  Run(std::shared_ptr<const Model::Model> model, std::vector<std::pair<double, UpdatingDataProvider::Producer>> updates)
    : model(model)
    , dataProvider(std::make_shared<UpdatingDataProvider>(model, csv, std::move(updates)))
    , engine(model)
  {
    dataProvider->setEndTime(5);
    entryHandler.connect(&engine);
    recorder.subscribe(&engine);
    engine.run(dataProvider->createScenario());
    objective = engine.getSystemState()->getObjective();
  }

  /// The times at which the conditional event of the given identifier completed.
  std::vector<double> completions(const std::string& nodeId) {
    std::vector<double> times;
    for ( auto& entry : recorder.find(nlohmann::json{{"nodeId", nodeId}, {"state", "COMPLETED"}}) ) {
      times.push_back(entry["status"]["timestamp"].get<double>());
    }
    return times;
  }
};

/// The identifier of the instance, computed when a test runs rather than at program start, when the string registry
/// it is looked up in may not yet exist.
BPMNOS::number instance1() {
  return BPMNOS::to_number(std::string("Instance_1"), STRING);
}

} // namespace

TEST_CASE( "A data update of a global attribute releases a conditional event waiting for it", "[execution][dataupdate]" ) {
  auto shared = model();
  auto flag = attributeOf(*shared, "Process_1", "flag");
  Run run(shared, { { 2, [flag](const Execution::SystemState* systemState) {
    return std::make_shared<Execution::DataUpdateEvent>(systemState, Execution::DataUpdateEvent::Values{ { flag, BPMNOS::number(1) } });
  } } });
  REQUIRE( run.completions("GlobalCondition_1") == std::vector<double>{ 2 } );
  REQUIRE( run.completions("DataCondition_1").empty() );
}

TEST_CASE( "A data update of the data of a process instance releases a conditional event and changes the objective", "[execution][dataupdate]" ) {
  auto shared = model();
  auto process = scopeOf(*shared, "Process_1");
  auto level = attributeOf(*shared, "Process_1", "level");
  SECTION( "An update satisfying the condition" ) {
    Run run(shared, { { 3, [process, level](const Execution::SystemState* systemState) {
      return std::make_shared<Execution::DataUpdateEvent>(systemState, process, instance1(), Execution::DataUpdateEvent::Values{ { level, BPMNOS::number(3) } });
    } } });
    REQUIRE( run.completions("DataCondition_1") == std::vector<double>{ 3 } );
    REQUIRE( run.completions("GlobalCondition_1").empty() );
    REQUIRE( run.objective == 6 );
  }
  SECTION( "An update not satisfying the condition" ) {
    Run run(shared, { { 3, [process, level](const Execution::SystemState* systemState) {
      return std::make_shared<Execution::DataUpdateEvent>(systemState, process, instance1(), Execution::DataUpdateEvent::Values{ { level, BPMNOS::number(2) } });
    } } });
    REQUIRE( run.completions("DataCondition_1").empty() );
    REQUIRE( run.objective == 4 );
  }
}

TEST_CASE( "A data update of the data of a subprocess reaches the state machine of the subprocess", "[execution][dataupdate]" ) {
  auto shared = model();
  auto subProcess = scopeOf(*shared, "SubProcess_1");
  auto inner = attributeOf(*shared, "SubProcess_1", "inner");
  Run run(shared, { { 1, [subProcess, inner](const Execution::SystemState* systemState) {
    return std::make_shared<Execution::DataUpdateEvent>(systemState, subProcess, instance1(), Execution::DataUpdateEvent::Values{ { inner, BPMNOS::number(1) } });
  } } });
  REQUIRE( run.completions("InnerCondition_1") == std::vector<double>{ 1 } );
}

TEST_CASE( "A data update of an instance that does not exist has expired and is discarded", "[execution][dataupdate]" ) {
  auto shared = model();
  auto process = scopeOf(*shared, "Process_1");
  auto level = attributeOf(*shared, "Process_1", "level");
  bool expired = false;
  Run run(shared, { { 3, [process, level, &expired](const Execution::SystemState* systemState) {
    auto event = std::make_shared<Execution::DataUpdateEvent>(systemState, process, BPMNOS::to_number(std::string("Instance_2"), STRING), Execution::DataUpdateEvent::Values{ { level, BPMNOS::number(3) } });
    expired = event->expired();
    return event;
  } } });
  REQUIRE( expired );
  REQUIRE( run.completions("DataCondition_1").empty() );
  REQUIRE( run.objective == 0 );
}

TEST_CASE( "A data update of an attribute that may not be updated is refused", "[execution][dataupdate]" ) {
  auto shared = model();
  auto process = scopeOf(*shared, "Process_1");
  auto subProcess = scopeOf(*shared, "SubProcess_1");
  using Values = Execution::DataUpdateEvent::Values;
  auto time = attributeOf(*shared, "Process_1", "time");
  auto flag = attributeOf(*shared, "Process_1", "flag");
  auto level = attributeOf(*shared, "Process_1", "level");
  auto fixed = attributeOf(*shared, "Process_1", "fixed");
  auto instance = attributeOf(*shared, "Process_1", "instance");
  auto inner = attributeOf(*shared, "SubProcess_1", "inner");
  auto timestamp = attributeOf(*shared, "Process_1", "timestamp");

  REQUIRE_THROWS_WITH( Execution::DataUpdateEvent(nullptr, Values{ { time, BPMNOS::number(1) } }),
    Catch::Matchers::ContainsSubstring("the time must not be updated") );
  REQUIRE_THROWS_WITH( Execution::DataUpdateEvent(nullptr, process, instance1(), Values{ { instance, BPMNOS::number(1) } }),
    Catch::Matchers::ContainsSubstring("the instance must not be updated") );
  REQUIRE_THROWS_WITH( Execution::DataUpdateEvent(nullptr, process, instance1(), Values{ { fixed, BPMNOS::number(2) } }),
    Catch::Matchers::ContainsSubstring("'Fixed' is immutable") );
  REQUIRE_THROWS_WITH( Execution::DataUpdateEvent(nullptr, process, instance1(), Values{ { timestamp, BPMNOS::number(2) } }),
    Catch::Matchers::ContainsSubstring("'Timestamp' is no data or global attribute") );
  REQUIRE_THROWS_WITH( Execution::DataUpdateEvent(nullptr, process, instance1(), Values{ { inner, BPMNOS::number(2) } }),
    Catch::Matchers::ContainsSubstring("'Inner' is no data attribute of 'Process_1'") );
  REQUIRE_THROWS_WITH( Execution::DataUpdateEvent(nullptr, subProcess, instance1(), Values{ { level, BPMNOS::number(2) } }),
    Catch::Matchers::ContainsSubstring("'Level' is no data attribute of 'SubProcess_1'") );
  REQUIRE_THROWS_WITH( Execution::DataUpdateEvent(nullptr, process, instance1(), Values{ { flag, BPMNOS::number(1) } }),
    Catch::Matchers::ContainsSubstring("'Flag' is no data attribute of 'Process_1'") );
  REQUIRE_THROWS_WITH( Execution::DataUpdateEvent(nullptr, Values{ { level, BPMNOS::number(2) } }),
    Catch::Matchers::ContainsSubstring("'Level' is no global attribute") );
}
