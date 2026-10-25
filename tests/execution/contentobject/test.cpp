#include "prelude.h"

namespace {

/// Replaces every occurrence of a placeholder in a text.
std::string fill(std::string text, const std::string& placeholder, const std::string& value) {
  for ( auto position = text.find(placeholder); position != std::string::npos; position = text.find(placeholder, position + value.size()) ) {
    text.replace(position, placeholder.size(), value);
  }
  return text;
}

/// Returns the operators with the given expressions.
std::string operatorsOf(const std::string& id, const std::vector<std::string>& expressions) {
  std::string operators;
  for ( size_t k = 0; k < expressions.size(); k++ ) {
    operators += "<bpmnos:operator id=\"" + id + std::to_string(k) + "\" expression=\"" + expressions[k] + "\" />\n";
  }
  return operators;
}

/// Two processes, the first throwing a message or signal named `@NAME@` by an intermediate throw event, the second
/// catching it by an intermediate catch event, each event applying the given operators.
std::string exchangeModel(const std::string& element, const std::string& name, const std::vector<std::string>& thrown, const std::vector<std::string>& caught) {
  std::string xml = R"xml(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:process id="Thrower" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp_Thrower" type="decimal" name="timestamp" />
          <bpmnos:attribute id="All" type="{ count: integer, pair: { a: integer, b: integer } }" name="all := { count := 3, pair := { a := 1, b := 2 } }" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_Thrower">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" type="string" name="instance" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_Thrower">
      <bpmn2:outgoing>Flow_1</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_1" sourceRef="Start_Thrower" targetRef="Throw" />
    <bpmn2:intermediateThrowEvent id="Throw">
      <bpmn2:extensionElements>
        <bpmnos:status>
          <bpmnos:operators>
            @THROWN@
          </bpmnos:operators>
        </bpmnos:status>
        <bpmnos:@ELEMENT@ name="@NAME@" type="{ count: integer, pair: { a: integer, b: integer } }" />
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_1</bpmn2:incoming>
      <bpmn2:outgoing>Flow_2</bpmn2:outgoing>
      <bpmn2:@ELEMENT@EventDefinition id="Definition_Thrower" />
    </bpmn2:intermediateThrowEvent>
    <bpmn2:sequenceFlow id="Flow_2" sourceRef="Throw" targetRef="End_Thrower" />
    <bpmn2:endEvent id="End_Thrower">
      <bpmn2:incoming>Flow_2</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
  <bpmn2:process id="Catcher" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp_Catcher" type="decimal" name="timestamp" />
          <bpmnos:attribute id="N" type="integer" name="n := 10" />
          <bpmnos:attribute id="First" type="integer" name="first" />
          <bpmnos:attribute id="Second" type="integer" name="second" />
          <bpmnos:attribute id="Other" type="integer" name="other" />
          <bpmnos:attribute id="Copy" type="{ count: integer, pair: { a: integer, b: integer } }" name="copy" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_Catcher">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" type="string" name="instance" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_Catcher">
      <bpmn2:outgoing>Flow_3</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_3" sourceRef="Start_Catcher" targetRef="Catch" />
    <bpmn2:intermediateCatchEvent id="Catch">
      <bpmn2:extensionElements>
        <bpmnos:status>
          <bpmnos:operators>
            @CAUGHT@
          </bpmnos:operators>
        </bpmnos:status>
        <bpmnos:@ELEMENT@ name="@NAME@" type="{ count: integer, pair: { a: integer, b: integer } }" />
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_3</bpmn2:incoming>
      <bpmn2:outgoing>Flow_4</bpmn2:outgoing>
      <bpmn2:@ELEMENT@EventDefinition id="Definition_Catcher" />
    </bpmn2:intermediateCatchEvent>
    <bpmn2:sequenceFlow id="Flow_4" sourceRef="Catch" targetRef="End_Catcher" />
    <bpmn2:endEvent id="End_Catcher">
      <bpmn2:incoming>Flow_4</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>
</bpmn2:definitions>
)xml";
  xml = fill(xml, "@THROWN@", operatorsOf("Thrown_", thrown));
  xml = fill(xml, "@CAUGHT@", operatorsOf("Caught_", caught));
  xml = fill(xml, "@ELEMENT@", element);
  return fill(xml, "@NAME@", name);
}

std::shared_ptr<const Model::Model> modelFrom(const std::string& xml) {
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

/// Runs the model, the catcher waiting before the thrower throws, and returns the record of the catch event when it
/// is departed, and the record of the message or signal.
std::pair<nlohmann::ordered_json, nlohmann::ordered_json> exchange(const std::string& element, const std::vector<std::string>& thrown, const std::vector<std::string>& caught) {
  auto model = modelFrom(exchangeModel(element, "Exchange", thrown, caught));
  auto provider = std::make_shared<Execution::StaticDataProvider>(model,
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "Instance_1; Thrower; timestamp := 1\n"
    "Instance_2; Catcher; timestamp := 0\n"
  );
  Execution::Engine engine(model);
  Execution::InstantEntry entryHandler;
  Execution::FirstMatchingMessageDelivery messageHandler;
  Execution::InstantExit exitHandler;
  entryHandler.connect(&engine);
  messageHandler.connect(&engine);
  exitHandler.connect(&engine);
  Execution::Recorder recorder;
  recorder.subscribe(&engine);
  engine.run(provider->createScenario());
  auto catchLog = recorder.find(nlohmann::json{{"nodeId","Catch"},{"state","DEPARTED"}});
  auto contentLog = ( element == "message" ?
    recorder.find(nlohmann::json{{"origin","Throw"},{"state","CREATED"}}) :
    recorder.find(nlohmann::json{{"name","Exchange"}}, nlohmann::json{{"event",nullptr}})
  );
  REQUIRE( catchLog.size() == 1 );
  REQUIRE( contentLog.size() == 1 );
  return { catchLog[0], contentLog[0] };
}

} // namespace

TEST_CASE( "The content object of messages and signals", "[execution][message][signal][content]" ) {
  for ( std::string element : { "message", "signal" } ) {
    DYNAMIC_SECTION( "A " << element ) {

      SECTION( "The content object written field by field and read field by field, compounded, as a whole and not at all" ) {
        auto [catchLog, contentLog] = exchange(element,
          { "Exchange.count := 3", "Exchange.pair.a := 1" },
          { "n += Exchange.count", "first := Exchange.pair.a", "second := Exchange.pair.b", "copy := Exchange", "other := 7" }
        );
        REQUIRE( contentLog["content"] == nlohmann::ordered_json::parse(R"({ "count": 3, "pair": { "a": 1, "b": null } })") );
        REQUIRE( catchLog["status"]["n"] == 13 );
        REQUIRE( catchLog["status"]["first"] == 1 );
        // a field no operator of the thrower assigns is undefined
        REQUIRE( catchLog["status"]["second"].is_null() );
        REQUIRE( catchLog["status"]["copy"] == nlohmann::ordered_json::parse(R"({ "count": 3, "pair": { "a": 1, "b": null } })") );
        REQUIRE( catchLog["status"]["other"] == 7 );
      }

      SECTION( "The content object written as a whole" ) {
        auto [catchLog, contentLog] = exchange(element, { "Exchange := all" }, { "copy := Exchange" });
        REQUIRE( contentLog["content"] == nlohmann::ordered_json::parse(R"({ "count": 3, "pair": { "a": 1, "b": 2 } })") );
        REQUIRE( catchLog["status"]["copy"] == nlohmann::ordered_json::parse(R"({ "count": 3, "pair": { "a": 1, "b": 2 } })") );
      }

      SECTION( "A catching node writing the content object is refused" ) {
        REQUIRE_THROWS_WITH(
          modelFrom(exchangeModel(element, "Exchange", {}, { "Exchange.count := 1" })),
          Catch::Matchers::ContainsSubstring("of a catching node writes the content")
        );
      }

      SECTION( "A name clashing with an attribute is refused" ) {
        REQUIRE_THROWS_WITH(
          modelFrom(exchangeModel(element, "n", {}, {})),
          Catch::Matchers::ContainsSubstring("duplicate attribute name 'n'")
        );
      }

      SECTION( "A name that is no valid name is refused" ) {
        REQUIRE_THROWS_WITH(
          modelFrom(exchangeModel(element, "an exchange", {}, {})),
          Catch::Matchers::ContainsSubstring("is no valid name")
        );
      }
    }
  }
}

namespace {

/// The exchange model with the content type `{ route: integer[] }`, the catcher copying the route.
std::string routeModel(const std::string& element, const std::vector<std::string>& thrown) {
  auto xml = exchangeModel(element, "Exchange", thrown, { "stops := Exchange.route" });
  xml = fill(xml, R"(type="{ count: integer, pair: { a: integer, b: integer } }" />)", R"(type="{ route: integer[] }" />)");
  return fill(xml, R"(<bpmnos:attribute id="Other" type="integer" name="other" />)", R"(<bpmnos:attribute id="Other" type="integer" name="other" />
          <bpmnos:attribute id="Stops" type="integer[]" name="stops" />)");
}

/// Runs the route model with the given size declaration and returns the record of the message or signal.
nlohmann::ordered_json routeExchange(const std::string& element, const std::vector<std::string>& thrown, const std::string& declaration) {
  auto model = modelFrom(routeModel(element, thrown));
  auto provider = std::make_shared<Execution::StaticDataProvider>(model,
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n" + declaration +
    "Instance_1; Thrower; timestamp := 1\n"
    "Instance_2; Catcher; timestamp := 0\n"
  );
  Execution::Engine engine(model);
  Execution::InstantEntry entryHandler;
  Execution::FirstMatchingMessageDelivery messageHandler;
  Execution::InstantExit exitHandler;
  entryHandler.connect(&engine);
  messageHandler.connect(&engine);
  exitHandler.connect(&engine);
  Execution::Recorder recorder;
  recorder.subscribe(&engine);
  engine.run(provider->createScenario());
  auto contentLog = ( element == "message" ?
    recorder.find(nlohmann::json{{"origin","Throw"},{"state","CREATED"}}) :
    recorder.find(nlohmann::json{{"name","Exchange"}}, nlohmann::json{{"event",nullptr}})
  );
  REQUIRE( contentLog.size() == 1 );
  return contentLog[0];
}

} // namespace

TEST_CASE( "Size declarations of the content of messages and signals", "[execution][message][signal][content]" ) {
  SECTION( "The content of a message declared at the throwing node" ) {
    auto contentLog = routeExchange("message", { "Exchange.route := [1, 2]" }, "; Throw; Exchange.route[4]\n");
    // the declared dimension is fixed, a shorter array being padded
    REQUIRE( contentLog["content"] == nlohmann::ordered_json::parse(R"({ "route": [1, 2, null, null] })") );
    REQUIRE_THROWS_WITH( routeExchange("message", { "Exchange.route := [1, 2, 3, 4, 5]" }, "; Throw; Exchange.route[4]\n"), Catch::Matchers::ContainsSubstring("5 elements where 4 are declared") );
  }

  SECTION( "The content of a message without a declaration takes the length assigned" ) {
    auto contentLog = routeExchange("message", { "Exchange.route := [1, 2]" }, "");
    REQUIRE( contentLog["content"] == nlohmann::ordered_json::parse(R"({ "route": [1, 2] })") );
  }

  SECTION( "The content of a signal declared by its name" ) {
    auto contentLog = routeExchange("signal", { "Exchange.route := [1, 2]" }, "; ; Exchange.route[4]\n");
    REQUIRE( contentLog["content"] == nlohmann::ordered_json::parse(R"({ "route": [1, 2, null, null] })") );
    // a signal raised by the environment starts from the same layout
    auto model = modelFrom(routeModel("signal", {}));
    auto provider = std::make_shared<Execution::StaticDataProvider>(model, "INSTANCE_ID; NODE_ID; INITIALIZATION\n; ; Exchange.route[4]\nInstance_1; Thrower;\n");
    REQUIRE( provider->getDefaultContent(BPMNOS::to_number(std::string("Exchange"),STRING))->values.size() == 4 );
  }

  SECTION( "Illegal size declarations of content are refused" ) {
    auto provide = [](const std::string& element, const std::string& declaration) {
      auto model = modelFrom(routeModel(element, {}));
      return std::make_shared<Execution::StaticDataProvider>(model, "INSTANCE_ID; NODE_ID; INITIALIZATION\n" + declaration + "Instance_1; Thrower;\n");
    };
    REQUIRE_THROWS_WITH( provide("signal", "; Throw; Exchange.route[4]\n"), Catch::Matchers::ContainsSubstring("the sizes of a signal being declared by its name") );
    REQUIRE_THROWS_WITH( provide("message", "; Catch; Exchange.route[4]\n"), Catch::Matchers::ContainsSubstring("must be given at the node throwing the content") );
    REQUIRE_THROWS_WITH( provide("message", "; ; Exchange.route[4]\n"), Catch::Matchers::ContainsSubstring("names no object or signal of the model") );
    REQUIRE_THROWS_WITH( provide("message", "; Throw; Other.route[4]\n"), Catch::Matchers::ContainsSubstring("names no object of node 'Throw'") );
  }
}
