#include "prelude.h"

TEST_CASE( "Parse message flows", "[model][parser]" ) {
  Model::Model model(std::string("tests/model/parser/Messaging.bpmn"));
  std::unordered_map< std::string, std::set< std::string > > candidates;

  for ( auto& process : model.processes ) {
    auto throwingMessageEvents = process->find_all(
      [](const BPMN::Node* node) { return node->represents<BPMN::MessageThrowEvent>();}
    );
    for ( auto throwingMessageEvent : throwingMessageEvents ) {
      for ( auto& candidate : throwingMessageEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates) {
        candidates[throwingMessageEvent->id].insert(candidate->id);
      }
    }
    auto catchingMessageEvents = process->find_all(
      [](const BPMN::Node* node) { return node->represents<BPMN::MessageCatchEvent>();}
    );
    for ( auto catchingMessageEvent : catchingMessageEvents ) {
      for ( auto& candidate : catchingMessageEvent->extensionElements->as<BPMNOS::Model::ExtensionElements>()->messageCandidates) {
        candidates[catchingMessageEvent->id].insert(candidate->id);
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

TEST_CASE( "Refuse a message instantiating a process that is caught elsewhere", "[model][parser]" ) {
  REQUIRE_THROWS_WITH(
    Model::Model(std::string("tests/model/parser/Triggering_message_caught_elsewhere.bpmn")),
    "Model: message 'Trigger' instantiates process 'Triggered' and is caught by 'CatchEvent_1'"
  );
}

TEST_CASE( "Refuse header parameters at a message start event of a process", "[model][parser]" ) {
  REQUIRE_THROWS_WITH(
    Model::Model(std::string("tests/model/parser/Triggering_message_with_header.bpmn")),
    "Model: message start event 'MessageStartEvent' of process 'Triggered' must not have header parameters"
  );
}

TEST_CASE( "Refuse header parameters at a node throwing a message instantiating a process", "[model][parser]" ) {
  REQUIRE_THROWS_WITH(
    Model::Model(std::string("tests/model/parser/Triggering_message_thrown_with_header.bpmn")),
    "Model: message 'Trigger' thrown by 'ThrowEvent_1' instantiates process 'Triggered' and must not have header parameters"
  );
}

TEST_CASE( "Refuse a message thrown within the process it instantiates", "[model][parser]" ) {
  REQUIRE_THROWS_WITH(
    Model::Model(std::string("tests/model/parser/Triggering_message_thrown_within_process.bpmn")),
    "Model: message 'Trigger' instantiating process 'Triggered' must not be thrown by 'ThrowEvent_1' of that process"
  );
}

TEST_CASE( "Refuse a typed compensation activity", "[model][parser]" ) {
  REQUIRE_THROWS_WITH(
    Model::Model(std::string("tests/model/parser/Typed_compensation_activity.bpmn")),
    "Model: compensation activity 'CompensationActivity_1' must not be a send, receive or decision task"
  );
}

TEST_CASE( "Refuse a gateway without incoming sequence flow", "[model][parser]" ) {
  REQUIRE_THROWS_WITH(
    Model::Model(std::string("tests/model/parser/Gateway_without_incoming_flow.bpmn")),
    "Model: gateway 'Gateway_1' has no incoming sequence flow"
  );
}

TEST_CASE( "Refuse a gateway without outgoing sequence flow", "[model][parser]" ) {
  REQUIRE_THROWS_WITH(
    Model::Model(std::string("tests/model/parser/Gateway_without_outgoing_flow.bpmn")),
    "Model: gateway 'Gateway_1' has no outgoing sequence flow"
  );
}

TEST_CASE( "Refuse an ad-hoc subprocess with a start event", "[model][parser]" ) {
  REQUIRE_THROWS_WITH(
    Model::Model(std::string("tests/model/parser/Adhoc_subprocess_with_start_event.bpmn")),
    "Model: start event provided for adhoc subprocess 'AdHocSubProcess_1'"
  );
}

TEST_CASE( "Refuse a signal event without a signal definition", "[model][parser]" ) {
  REQUIRE_THROWS_WITH(
    Model::Model(std::string("tests/model/parser/Signal_without_definition.bpmn")),
    "Model: No signal defined for signal throw event 'ThrowEvent_1'"
  );
}
