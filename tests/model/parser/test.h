TEST_CASE( "Parse BPMN elements", "[model][parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("model/parser/Model_without_extensions.bpmn")) );
}

TEST_CASE( "Parse empty custom activity", "[model][parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("model/parser/EmptyCustomActivity.bpmn")) );
}

TEST_CASE( "Parse simplified resource activity", "[model][parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("model/parser/SimplifiedResourceActivity.bpmn")) );
}

TEST_CASE( "Parse request activity", "[model][parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("model/parser/RequestActivity.bpmn")) );
}

TEST_CASE( "Parse release activity", "[model][parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("model/parser/ReleaseActivity.bpmn")) );
}

TEST_CASE( "Parse resource activity", "[model][parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("model/parser/ResourceActivity.bpmn")) );
}
