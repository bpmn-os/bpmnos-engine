#include "prelude.h"

namespace {

std::shared_ptr<const Model::Model> modelFrom(const std::string& xml) {
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

/// Returns the attribute `type` stating the given type, or nothing if no type is given.
std::string typeOf(const std::string& type) {
  return type.empty() ? std::string() : " type=\"" + type + "\"";
}

/// Two processes, the first throwing a message or signal by an intermediate throw event, the second catching it by an
/// intermediate catch event, each element stating the given definition and content.
std::string exchangeModel(const std::string& element, const std::string& definition, const std::string& thrown, const std::string& caught) {
  auto eventDefinition = ( element == "message" ? "messageEventDefinition" : "signalEventDefinition" );
  auto process = [&](const std::string& id, const std::string& event, const std::string& definitionAndContent) {
    return R"xml(
  <bpmn2:process id=")xml" + id + R"xml(" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp_)xml" + id + R"xml(" type="decimal" name="timestamp" />
          <bpmnos:attribute id="Value_)xml" + id + R"xml(" type="integer" name="value" />
        </bpmnos:attributes>
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_)xml" + id + R"xml(">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" type="string" name="instance" />
        </bpmnos:attributes>
      </bpmn2:extensionElements>
    </bpmn2:dataObject>
    <bpmn2:startEvent id="Start_)xml" + id + R"xml(">
      <bpmn2:outgoing>Flow_A_)xml" + id + R"xml(</bpmn2:outgoing>
    </bpmn2:startEvent>
    <bpmn2:sequenceFlow id="Flow_A_)xml" + id + R"xml(" sourceRef="Start_)xml" + id + R"xml(" targetRef=")xml" + id + R"xml(_Event" />
    <bpmn2:)xml" + event + R"xml( id=")xml" + id + R"xml(_Event">
      <bpmn2:extensionElements>
        )xml" + definitionAndContent + R"xml(
      </bpmn2:extensionElements>
      <bpmn2:incoming>Flow_A_)xml" + id + R"xml(</bpmn2:incoming>
      <bpmn2:outgoing>Flow_B_)xml" + id + R"xml(</bpmn2:outgoing>
      <bpmn2:)xml" + eventDefinition + R"xml( id="Definition_)xml" + id + R"xml(" />
    </bpmn2:)xml" + event + R"xml(>
    <bpmn2:sequenceFlow id="Flow_B_)xml" + id + R"xml(" sourceRef=")xml" + id + R"xml(_Event" targetRef="End_)xml" + id + R"xml(" />
    <bpmn2:endEvent id="End_)xml" + id + R"xml(">
      <bpmn2:incoming>Flow_B_)xml" + id + R"xml(</bpmn2:incoming>
    </bpmn2:endEvent>
  </bpmn2:process>)xml";
  };
  return R"xml(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">)xml"
    + process("Thrower", "intermediateThrowEvent", "<bpmnos:" + element + " name=\"" + definition + "\"" + thrown + "</bpmnos:" + element + ">")
    + process("Catcher", "intermediateCatchEvent", "<bpmnos:" + element + " name=\"" + definition + "\"" + caught + "</bpmnos:" + element + ">")
    + "\n</bpmn2:definitions>\n";
}

/// The rest of the definition of a message or signal following its name: its type, if any, and its content.
std::string stated(const std::string& type, const std::string& content = "") {
  return typeOf(type) + ">" + content;
}

} // namespace

TEST_CASE( "Definitions of messages and signals", "[model][definitions]" ) {
  for ( std::string element : { "message", "signal" } ) {
    DYNAMIC_SECTION( "A " << element ) {

      SECTION( "Types agreeing regardless of spacing and of the order of fields, at every depth, are accepted" ) {
        REQUIRE_NOTHROW( modelFrom(exchangeModel(element, "Exchange",
          stated("{ a: integer, b: { x: decimal, y: string }, c: { p: integer, q: boolean }[2] }"),
          stated("{c:{q:boolean,p:integer}[2],b:{y:string,x:decimal},a:integer}")
        )) );
      }

      SECTION( "Types differing in a field, the type of a field or the size of a dimension are refused" ) {
        for ( std::string caught : { "{ a: integer }", "{ a: integer, b: decimal }", "{ a: integer, b: integer[3] }" } ) {
          REQUIRE_THROWS_WITH(
            modelFrom(exchangeModel(element, "Exchange", stated("{ a: integer, b: integer[2] }"), stated(caught))),
            Catch::Matchers::ContainsSubstring("must have the same type")
          );
        }
      }

      SECTION( "A definition without a type disagrees with one stating fields" ) {
        REQUIRE_THROWS_WITH(
          modelFrom(exchangeModel(element, "Exchange", stated("{ a: integer }"), stated(""))),
          Catch::Matchers::ContainsSubstring("must have the same type")
        );
      }

      SECTION( "Definitions without a type agree" ) {
        REQUIRE_NOTHROW( modelFrom(exchangeModel(element, "Exchange", stated(""), stated(""))) );
      }

      SECTION( "A scalar type is refused" ) {
        REQUIRE_THROWS_WITH(
          modelFrom(exchangeModel(element, "Exchange", stated("integer"), stated("integer"))),
          Catch::Matchers::ContainsSubstring("must be an object")
        );
      }

      SECTION( "A content key is a field of the type" ) {
        REQUIRE_NOTHROW( modelFrom(exchangeModel(element, "Exchange",
          stated("{ value: integer }", "<bpmnos:content key=\"value\" attribute=\"value\" />"),
          stated("{ value: integer }", "<bpmnos:content key=\"value\" attribute=\"value\" />")
        )) );
        REQUIRE_THROWS_WITH(
          modelFrom(exchangeModel(element, "Exchange",
            stated("{ value: integer }", "<bpmnos:content key=\"other\" attribute=\"value\" />"),
            stated("{ value: integer }")
          )),
          Catch::Matchers::ContainsSubstring("is no field of its type")
        );
      }
    }
  }

  SECTION( "A message and a signal of different names may have different types" ) {
    // the catcher states another name, so that the two are no candidates of each other
    auto xml = exchangeModel("message", "Exchange", stated("{ a: integer }"), stated("{ b: integer }"));
    xml.replace(xml.rfind("name=\"Exchange\""), 15, "name=\"Other\"");
    auto model = modelFrom(xml);
    auto catcher = model->processes.back()->find([](BPMN::Node* node) { return node->id == "Catcher_Event"; });
    REQUIRE( catcher->extensionElements->as<Model::ExtensionElements>()->messageCandidates.empty() );
  }

  SECTION( "A header parameter named 'name' is refused" ) {
    REQUIRE_THROWS_WITH(
      modelFrom(exchangeModel("message", "Exchange", stated("", "<bpmnos:parameter name=\"name\" value=\"&#34;Exchange&#34;\" />"), stated(""))),
      Catch::Matchers::ContainsSubstring("must not have a header parameter 'name'")
    );
  }

  SECTION( "The header begins with the sender and the recipient" ) {
    auto model = modelFrom(exchangeModel("message", "Exchange", stated(""), stated("")));
    auto thrower = model->processes.front()->find([](BPMN::Node* node) { return node->id == "Thrower_Event"; });
    auto& header = thrower->extensionElements->as<Model::ExtensionElements>()->getMessageDefinition()->header;
    REQUIRE( header.size() == 2 );
    REQUIRE( header[Model::MessageDefinition::Index::Sender].key == "sender" );
    REQUIRE( header[Model::MessageDefinition::Index::Recipient].key == "recipient" );
  }
}
