#include "prelude.h"

namespace {

/// The parts of a test model into which attributes and further declarations are inserted.
struct Declarations {
  std::string globals;
  std::string processStatus;
  std::string processRestrictions;
  std::string processData;
  std::string subProcessData;
  std::string taskStatus;
  std::string taskDecisions;
};

/// Returns a model with a global data store, a process with a subprocess holding a decision task, and the
/// given declarations.
std::unique_ptr<Model::Model> modelWith(const Declarations& declarations) {
  std::string xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL" xmlns:bpmnos="https://bpmnos.telematique.eu" id="Definitions_1" targetNamespace="http://bpmn.io/schema/bpmn">
  <bpmn2:process id="Process_1" isExecutable="true">
    <bpmn2:extensionElements>
      <bpmnos:status>
        <bpmnos:attributes>
          <bpmnos:attribute id="Timestamp" name="timestamp" type="decimal" />)" + declarations.processStatus + R"(
        </bpmnos:attributes>)" + declarations.processRestrictions + R"(
      </bpmnos:status>
    </bpmn2:extensionElements>
    <bpmn2:dataObject id="DataObject_1">
      <bpmn2:extensionElements>
        <bpmnos:attributes>
          <bpmnos:attribute id="Instance" name="instance" type="string" />)" + declarations.processData + R"(
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
            <bpmnos:attribute id="Z" name="z" type="integer" />)" + declarations.subProcessData + R"(
          </bpmnos:attributes>
        </bpmn2:extensionElements>
      </bpmn2:dataObject>
      <bpmn2:startEvent id="Start_2">
        <bpmn2:outgoing>Flow_3</bpmn2:outgoing>
      </bpmn2:startEvent>
      <bpmn2:sequenceFlow id="Flow_3" sourceRef="Start_2" targetRef="Task_1" />
      <bpmn2:task id="Task_1" bpmnos:type="Decision">
        <bpmn2:extensionElements>
          <bpmnos:status>
            <bpmnos:attributes>
              <bpmnos:attribute id="Choice" name="choice" type="integer" />)" + declarations.taskStatus + R"(
            </bpmnos:attributes>
            <bpmnos:decisions>
              <bpmnos:decision id="Decision_1" condition=")" + ( declarations.taskDecisions.empty() ? "1 &#60;= choice &#60;= 3" : declarations.taskDecisions ) + R"(" />
            </bpmnos:decisions>
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
        <bpmnos:attribute id="Budget" name="budget" type="decimal" />)" + declarations.globals + R"(
      </bpmnos:attributes>
    </bpmn2:extensionElements>
  </bpmn2:dataStore>
</bpmn2:definitions>
)";
  auto root = XML::XMLObject::createFromString(xml);
  if ( !root ) {
    throw std::runtime_error("test: failed to parse model");
  }
  return std::make_unique<Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

std::string attribute(const std::string& id, const std::string& name, const std::string& type, const std::string& further = "") {
  return "\n<bpmnos:attribute id=\"" + id + "\" name=\"" + name + "\" type=\"" + type + "\" " + further + "/>";
}

} // namespace

TEST_CASE( "Parse the schema of an attribute", "[model][schema]" ) {
  using Model::Schema;

  SECTION( "Scalars" ) {
    for ( auto [text, type] : std::vector< std::pair<std::string, ValueType> >{
      {"boolean", BOOLEAN}, {"integer", INTEGER}, {"decimal", DECIMAL}, {"string", STRING}, {"collection", COLLECTION}
    } ) {
      auto schema = Schema::parse(text);
      REQUIRE( schema.isScalar() );
      REQUIRE( schema.scalar == type );
      REQUIRE( schema.stringify() == text );
    }
  }

  SECTION( "Arrays with fixed and open dimensions in index order" ) {
    auto schema = Schema::parse(" boolean [3][ ] [4]");
    REQUIRE( !schema.isScalar() );
    REQUIRE( schema.scalar == BOOLEAN );
    REQUIRE( schema.dimensions == std::vector< std::optional<size_t> >{ 3, std::nullopt, 4 } );
    REQUIRE( schema.stringify() == "boolean[3][][4]" );
  }

  SECTION( "Objects with nested fields" ) {
    auto schema = Schema::parse("{ cost: decimal, flags: boolean[2], position: { x: decimal, y: decimal } }[]");
    REQUIRE( !schema.isScalar() );
    REQUIRE( !schema.scalar.has_value() );
    REQUIRE( schema.dimensions == std::vector< std::optional<size_t> >{ std::nullopt } );
    REQUIRE( schema.fields.size() == 3 );
    REQUIRE( schema.fields[0].first == "cost" );
    REQUIRE( schema.fields[0].second.isScalar() );
    REQUIRE( schema.fields[1].second.dimensions == std::vector< std::optional<size_t> >{ 2 } );
    REQUIRE( schema.fields[2].second.fields.size() == 2 );
    REQUIRE( schema.stringify() == "{ cost: decimal, flags: boolean[2], position: { x: decimal, y: decimal } }[]" );
  }

  SECTION( "Illegal schemas" ) {
    for ( std::string text : {
      "", "float", "integer[0]", "integer[x]", "integer[3", "integer]", "{}", "{ cost }", "{ cost: decimal, cost: integer }",
      "{ cost: decimal", "{ 1cost: decimal }", "integer[3] extra"
    } ) {
      INFO( text );
      REQUIRE_THROWS_WITH( Schema::parse(text), Catch::Matchers::StartsWith("Schema: illegal type") );
    }
  }
}

TEST_CASE( "Number objects separately from scalar attributes", "[model][schema]" ) {
  Declarations declarations;
  declarations.globals = attribute("Location", "location", "integer[3]") + attribute("Limit", "limit", "integer");
  declarations.processStatus = attribute("Route", "route", "integer[]") + attribute("X", "x", "decimal");
  declarations.processData = attribute("Facilities", "facilities", "{ cost: decimal, flags: boolean[2] }[]") + attribute("Y", "y", "decimal");
  declarations.subProcessData = attribute("Grid", "grid", "boolean[3][4]");
  auto model = modelWith(declarations);

  // the scalar globals precede the instance, the global object being numbered separately
  REQUIRE( model->attributes.size() == 2 );
  REQUIRE( model->objects.size() == 1 );
  REQUIRE( model->instanceIndex == 2 );

  auto& process = model->processes.front();
  auto processExtension = process->extensionElements->as<Model::ExtensionElements>();
  auto& registry = processExtension->attributeRegistry;
  REQUIRE( registry["budget"]->index == 0 );
  REQUIRE( registry["limit"]->index == 1 );
  REQUIRE( registry["instance"]->index == 2 );
  REQUIRE( registry["y"]->index == 3 );
  REQUIRE( registry["timestamp"]->index == 0 );
  REQUIRE( registry["x"]->index == 1 );

  REQUIRE( registry["location"]->isObject() );
  REQUIRE( registry["location"]->index == 0 );
  REQUIRE( registry["facilities"]->index == 1 );
  REQUIRE( registry["route"]->isObject() );
  REQUIRE( registry["route"]->index == 0 );
  REQUIRE( registry.dataObjects.size() == 2 );
  REQUIRE( registry.statusObjects.size() == 1 );
  REQUIRE( registry["facilities"]->schema->stringify() == "{ cost: decimal, flags: boolean[2] }[]" );

  // the scope declares its own objects apart from its scalar attributes
  REQUIRE( processExtension->statusObjects.size() == 1 );
  REQUIRE( processExtension->dataObjects.size() == 1 );
  REQUIRE( processExtension->attributes.size() == 2 );
  REQUIRE( processExtension->data.size() == 2 );

  // a subprocess numbers its objects after those of its enclosing scopes
  auto subProcess = process->find_all([](const BPMN::Node* node) { return node->id == "SubProcess_1"; }).front();
  auto subProcessExtension = subProcess->extensionElements->as<Model::ExtensionElements>();
  REQUIRE( subProcessExtension->attributeRegistry["grid"]->index == 2 );
  REQUIRE( subProcessExtension->attributeRegistry["z"]->index == 4 );
  REQUIRE( subProcessExtension->dataObjects.size() == 1 );
  REQUIRE( subProcessExtension->data.size() == 1 );
}

TEST_CASE( "Refuse illegal uses of objects", "[model][schema]" ) {
  SECTION( "Unknown type" ) {
    REQUIRE_THROWS_WITH( []() { Declarations declarations; declarations.processData = attribute("Y", "y", "float"); return modelWith(declarations); }(), Catch::Matchers::ContainsSubstring("illegal type of attribute 'Y'") );
  }
  SECTION( "Objective on an object" ) {
    REQUIRE_THROWS_WITH( []() { Declarations declarations; declarations.processData = attribute("Y", "y", "decimal[2]", "objective=\"maximize\" weight=\"1\""); return modelWith(declarations); }(), Catch::Matchers::ContainsSubstring("requires type boolean, integer, or decimal") );
  }
  SECTION( "Objective on a string" ) {
    REQUIRE_THROWS_WITH( []() { Declarations declarations; declarations.processData = attribute("Y", "y", "string", "objective=\"maximize\" weight=\"1\""); return modelWith(declarations); }(), Catch::Matchers::ContainsSubstring("requires type boolean, integer, or decimal") );
  }
  SECTION( "Initial value of an object that is no literal" ) {
    REQUIRE_THROWS_WITH( []() { Declarations declarations; declarations.processData = attribute("Y", "y := 1", "decimal[2]"); return modelWith(declarations); }(), Catch::Matchers::ContainsSubstring("must be initialised with a literal") );
  }
  SECTION( "Object in an expression" ) {
    REQUIRE_THROWS_WITH( []() { Declarations declarations; declarations.processRestrictions = R"(
        <bpmnos:restrictions>
          <bpmnos:restriction id="Restriction_1" expression="location > 0" />
        </bpmnos:restrictions>)"; declarations.processData = attribute("Location", "location", "integer[3]"); return modelWith(declarations); }(), Catch::Matchers::ContainsSubstring("object 'location' cannot be used") );
  }
  SECTION( "Object as the attribute of a choice" ) {
    REQUIRE_THROWS_WITH( []() { Declarations declarations; declarations.taskStatus = attribute("Pick", "pick", "integer[2]"); declarations.taskDecisions = "1 &#60;= pick &#60;= 3"; return modelWith(declarations); }(), Catch::Matchers::ContainsSubstring("is an object") );
  }
}
