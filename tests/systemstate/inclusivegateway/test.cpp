#include "prelude.h"

SCENARIO( "SystemState copy with token awaiting inclusive gateway activation", "[systemstate][inclusivegateway]" ) {
  const std::string modelFile = "tests/systemstate/inclusivegateway/Fork-Join.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with a token waiting at a converging inclusive gateway" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    Execution::Engine engine(model);
    // No entry handler - the task does not proceed and blocks the merge, at which the token of the direct path waits
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    dataProvider->setEndTime(0);
    engine.run(std::move(scenario), 0);
    const auto* originalState = engine.getSystemState();

    // one token waits at Gateway_2, the token at Activity_1 being at a predecessor
    REQUIRE( originalState->tokensAwaitingGatewayActivation.size() == 1 );
    auto& [sm, gatewayMap] = *originalState->tokensAwaitingGatewayActivation.begin();
    REQUIRE( gatewayMap.size() == 1 );
    auto& [gatewayNode, waitingTokens] = *gatewayMap.begin();
    REQUIRE( gatewayNode->id == "Gateway_2" );
    REQUIRE( waitingTokens.size() == 1 );
    REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).empty() );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider->createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same tokens awaiting gateway activation" ) {
        REQUIRE( copiedState.tokensAwaitingGatewayActivation.size() ==
                 originalState->tokensAwaitingGatewayActivation.size() );
        auto& [copiedSM, copiedGatewayMap] = *copiedState.tokensAwaitingGatewayActivation.begin();
        REQUIRE( copiedGatewayMap.size() == gatewayMap.size() );
        auto& [copiedGatewayNode, copiedWaitingTokens] = *copiedGatewayMap.begin();
        REQUIRE( copiedWaitingTokens.size() == waitingTokens.size() );
        REQUIRE( copiedGatewayNode == gatewayNode );
      }
    }
  }
}
