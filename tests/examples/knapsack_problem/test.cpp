#include "prelude.h"
#include "golden/golden.h"

SCENARIO( "Knapsack problem", "[examples][knapsack_problem]" ) {
  const std::string modelFile = "examples/knapsack_problem/Knapsack_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One knapsack and three items" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Knapsack1; KnapsackProcess; items := 3\n"
      "Knapsack1; KnapsackProcess; capacity := 40\n"
      "Item1; ItemProcess; weight := 20\n"
      "Item1; ItemProcess; value := 100\n"
      "Item2; ItemProcess; weight := 15\n"
      "Item2; ItemProcess; value := 50\n"
      "Item3; ItemProcess; weight := 22\n"
      "Item3; ItemProcess; value := 120\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with naive dispatcher" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry parallelEntryHandler;
//      Execution::FirstComeFirstServedSequentialEntry sequentialEntryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      parallelEntryHandler.connect(&engine);
//      sequentialEntryHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      goldenMaster("knapsack_problem-1", recorder, engine);
      THEN( "The run terminates without failure and some items are accepted, some are rejected" ) {
        auto failureLog = recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ItemRejected"}}).size() > 0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ItemAccepted"}}).size() > 0);
      }
    }
  }
}

