#include "prelude.h"
#include <map>

namespace {

/// Two processes: the first sends its route and its depot and then writes its route, the second receives them in
/// its status, a message catch event writing no data.
std::string messagingModel(const std::string& stopsType) {
  return R"xml(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:collaboration id="Collaboration_1">
    <bpmn2:participant id="Participant_1" processRef="Process_1" />
    <bpmn2:participant id="Participant_2" processRef="Process_2" />
  </bpmn2:collaboration>
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" type="decimal" name="timestamp" />
          <bpmnos:attribute id="Route" type="integer[]" name="route" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" type="string" name="instance" />
          <bpmnos:attribute id="Depot" type="{ x: decimal }" name="depot := { x := 3 }" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_1">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_1" targetRef="Throw_1" />
    <bpmn2:intermediateThrowEvent id="Throw_1">
      <bpmn2:extensionElements>
        <bpmnos:message name="Message">
          <bpmnos:parameter name="recipient" value="&#34;Instance_2&#34;" />
          <bpmnos:content key="Route" attribute="route" />
          <bpmnos:content key="Depot" attribute="depot" />
        </bpmnos:message>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_2</bpmn2:outgoing>
      <bpmn2:messageEventDefinition id="MessageEventDefinition_1" />
    </bpmn2:intermediateThrowEvent>
    <bpmn2:sequenceFlow id="Flow_2" sourceRef="Throw_1" targetRef="Task_1" />
    <bpmn2:task id="Task_1">
      <bpmn2:extensionElements>
        <bpmnos:status>
          <bpmnos:operators>
            <bpmnos:operator id="Operator_1" expression="route[1] := 9" />
          </bpmnos:operators>
        </bpmnos:status>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_2</bpmn2:incoming>
      <bpmn2:outgoing>Flow_3</bpmn2:outgoing>
    </bpmn2:task>
    <bpmn2:sequenceFlow id="Flow_3" sourceRef="Task_1" targetRef="End_1" />
    <bpmn2:endEvent id="End_1">
      <bpmn2:incoming>Flow_3</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
  <bpmn2:process id="Process_2" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" type="decimal" name="timestamp" />
          <bpmnos:attribute id="Stops" type=")xml" + stopsType + R"xml(" name="stops" />
          <bpmnos:attribute id="Place" type="{ x: decimal }" name="place" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_2">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" type="string" name="instance" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_2">
      <bpmn2:outgoing>Flow_4</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_4" sourceRef="Start_2" targetRef="Catch_2" />
    <bpmn2:intermediateCatchEvent id="Catch_2">
      <bpmn2:extensionElements>
        <bpmnos:message name="Message">
          <bpmnos:content key="Route" attribute="stops" />
          <bpmnos:content key="Depot" attribute="place" />
        </bpmnos:message>
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_4</bpmn2:incoming>
      <bpmn2:outgoing>Flow_5</bpmn2:outgoing>
      <bpmn2:messageEventDefinition id="MessageEventDefinition_2" />
    </bpmn2:intermediateCatchEvent>
    <bpmn2:sequenceFlow id="Flow_5" sourceRef="Catch_2" targetRef="End_2" />
    <bpmn2:endEvent id="End_2">
      <bpmn2:incoming>Flow_5</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
</bpmn2:definitions>
)xml";
}

std::shared_ptr<const Model::Model> modelFrom(const std::string& xml) {
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

/// Records a copy of the status and data of every token at every node and state.
struct Capture : Execution::Observer {
  std::map<std::pair<std::string, std::string>, std::pair<BPMNOS::Status, BPMNOS::Data>> states;
  void notice(const Execution::Observable* observable) override {
    auto token = static_cast<const Execution::Token*>(observable);
    if ( !token->node->represents<BPMN::FlowNode>() ) {
      return;
    }
    states[{token->node->id, Execution::Token::stateName[(int)token->state]}] = { token->status, BPMNOS::Data(*token->data) };
  }
};

const std::string instances =
  "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
  "Instance_1; Process_1; route := [1, 2]\n"
  "Instance_2; Process_2;\n";

void run(const std::shared_ptr<const Model::Model>& model, Capture& capture) {
  auto provider = std::make_shared<Execution::StaticDataProvider>(model, instances);
  Execution::Engine engine(model);
  Execution::InstantEntry entryHandler;
  Execution::FirstMatchingMessageDelivery messageHandler;
  Execution::InstantExit exitHandler;
  messageHandler.connect(&engine);
  entryHandler.connect(&engine);
  exitHandler.connect(&engine);
  engine.addSubscriber(&capture, Execution::Observable::Type::Token);
  engine.run(provider->createScenario());
}

const Model::AttributeRegistry& registryOf(const Model::Model& model, size_t process) {
  return model.processes.at(process)->extensionElements->as<Model::ExtensionElements>()->attributeRegistry;
}

} // namespace

TEST_CASE( "A message carries arrays and objects", "[execution][message][objects]" ) {

  SECTION( "The receiver takes the objects sent, padded at fixed dimensions, unaffected by later writes" ) {
    auto model = modelFrom(messagingModel("integer[3]"));
    Capture capture;
    run(model, capture);

    auto& receiver = registryOf(*model, 1);
    auto& [status, data] = capture.states.at({"End_2", "DONE"});
    REQUIRE( to_string(*receiver.getObject(receiver["stops"], status, data)) == "[ 1, 2, undefined ]" );
    REQUIRE( to_string(*receiver.getObject(receiver["place"], status, data)) == "{ x := 3 }" );

    // the sender wrote its route after sending, which the receiver does not see
    auto& sender = registryOf(*model, 0);
    auto& [senderStatus, senderData] = capture.states.at({"End_1", "DONE"});
    REQUIRE( to_string(*sender.getObject(sender["route"], senderStatus, senderData)) == "[ 9, 2 ]" );

    // an object of the very layout is shared with the sender
    REQUIRE( receiver.getObject(receiver["place"], status, data).get() == sender.getObject(sender["depot"], senderStatus, senderData).get() );
  }

  SECTION( "An array longer than a fixed dimension is refused" ) {
    auto model = modelFrom(messagingModel("integer[1]"));
    Capture capture;
    REQUIRE_THROWS_WITH( run(model, capture), Catch::Matchers::ContainsSubstring("2 elements where 1 are declared") );
  }

  SECTION( "A key that is an object on one side only is refused when the model is parsed" ) {
    REQUIRE_THROWS_WITH( modelFrom(messagingModel("integer")), Catch::Matchers::ContainsSubstring("must be an object on both sides or on neither") );
  }
}

TEST_CASE( "Content is applied to the receiving attribute", "[execution][message][objects]" ) {
  auto model = modelFrom(messagingModel("integer[]"));
  auto& registry = registryOf(*model, 1);
  auto provider = std::make_shared<Execution::StaticDataProvider>(model, instances);
  auto scenario = provider->createScenario();
  BPMNOS::Status status( registry.statusAttributes.size() );
  status.objects.resize( registry.statusObjects.size() );
  auto stops = registry["stops"];
  status.objects[stops->index] = Model::Schema::parse("integer[]").undefinedObject();
  BPMNOS::Data data = provider->getGlobals(*scenario);

  SECTION( "Text raised from outside the model states an array as a literal" ) {
    Execution::ContentMap::mapped_type content = std::string("[4, 5]");
    Execution::applyContent(registry, "Route", stops, &content, status, data);
    REQUIRE( to_string(*status.objects[stops->index]) == "[ 4, 5 ]" );
  }
  SECTION( "Text that is no literal is refused for an object" ) {
    Execution::ContentMap::mapped_type content = std::string("4");
    REQUIRE_THROWS_WITH( Execution::applyContent(registry, "Route", stops, &content, status, data), Catch::Matchers::ContainsSubstring("is no literal") );
  }
  SECTION( "A value is refused for an object" ) {
    Execution::ContentMap::mapped_type content = BPMNOS::Value(BPMNOS::number(4));
    REQUIRE_THROWS_WITH( Execution::applyContent(registry, "Route", stops, &content, status, data), Catch::Matchers::ContainsSubstring("a value is sent") );
  }
  SECTION( "An object the content does not mention keeps its value" ) {
    auto before = status.objects[stops->index];
    Execution::applyContent<BPMNOS::Data>(registry, "Route", stops, nullptr, status, data);
    REQUIRE( status.objects[stops->index] == before );
  }
}
