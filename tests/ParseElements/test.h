TEST_CASE( "Parse BPMN elements", "[parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("ParseElements/Model_without_extensions.bpmn")) );
}

TEST_CASE( "Parse empty custom activity", "[parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("ParseElements/EmptyCustomActivity.bpmn")) );
}

TEST_CASE( "Parse simplified resource activity", "[parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("ParseElements/SimplifiedResourceActivity.bpmn")) );
}

TEST_CASE( "Parse request activity", "[parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("ParseElements/RequestActivity.bpmn")) );
}

TEST_CASE( "Parse release activity", "[parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("ParseElements/ReleaseActivity.bpmn")) );
}

TEST_CASE( "Parse resource activity", "[parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("ParseElements/ResourceActivity.bpmn")) );
}
