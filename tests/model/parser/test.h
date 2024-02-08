TEST_CASE( "Parse BPMN elements", "[model][parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("model/parser/Model_without_extensions.bpmn")) );
}

TEST_CASE( "Parse empty custom activity", "[model][parser]" ) {
  REQUIRE_NOTHROW( Model::Model(std::string("model/parser/EmptyCustomActivity.bpmn")) );
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

TEST_CASE( "Parse message flows", "[model][parser]" ) {
  Model::Model model(std::string("model/parser/Messaging.bpmn"));
  std::unordered_map< std::string, std::set< std::string > > candidates;

  for ( auto& process : model.processes ) {
    auto throwingMessageEvents = process->find_all(
      [](const BPMN::Node* node) { return node->represents<BPMN::MessageThrowEvent>();}
    );
    for ( auto throwingMessageEvent : throwingMessageEvents ) {
      for ( auto& message : throwingMessageEvent->extensionElements->as<BPMNOS::Model::Status>()->messageDefinitions) {
        for ( auto candidate : message->candidates) {
          candidates[throwingMessageEvent->id].insert(candidate->id);
        }
      }
    }
    auto catchingMessageEvents = process->find_all(
      [](const BPMN::Node* node) { return node->represents<BPMN::MessageCatchEvent>();}
    );
    for ( auto catchingMessageEvent : catchingMessageEvents ) {
      for ( auto& message : catchingMessageEvent->extensionElements->as<BPMNOS::Model::Status>()->messageDefinitions) {
        for ( auto candidate : message->candidates) {
          candidates[catchingMessageEvent->id].insert(candidate->id);
        }
      }
    }
  }

  REQUIRE( candidates["CatchEvent_A1"] == std::set< std::string >({"ThrowEvent_B2","ThrowEvent_C2"}) );
  REQUIRE( candidates["CatchEvent_A2"] == std::set< std::string >({"ThrowEvent_B1","ThrowEvent_B2"}) );
  REQUIRE( candidates["CatchEvent_B1"] == std::set< std::string >({"ThrowEvent_A1"}) );
  REQUIRE( candidates["CatchEvent_B2"] == std::set< std::string >({"ThrowEvent_C1"}) );
  REQUIRE( candidates["CatchEvent_C1"] == std::set< std::string >() );
  REQUIRE( candidates["CatchEvent_C2"] == std::set< std::string >({"ThrowEvent_A2"}) );

  REQUIRE( candidates["ThrowEvent_A1"] == std::set< std::string >({"CatchEvent_B1"}) );
  REQUIRE( candidates["ThrowEvent_A2"] == std::set< std::string >({"CatchEvent_C2"}) );
  REQUIRE( candidates["ThrowEvent_B1"] == std::set< std::string >({"CatchEvent_A2"}) );
  REQUIRE( candidates["ThrowEvent_B2"] == std::set< std::string >({"CatchEvent_A1","CatchEvent_A2"}) );
  REQUIRE( candidates["ThrowEvent_C1"] == std::set< std::string >({"CatchEvent_B2"}) );
  REQUIRE( candidates["ThrowEvent_C2"] == std::set< std::string >({"CatchEvent_A1"}) );

}
