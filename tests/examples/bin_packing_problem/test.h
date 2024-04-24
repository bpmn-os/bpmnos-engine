SCENARIO( "Bin packing problem", "[examples][bin_packing_problem]" ) {
  const std::string modelFile = "examples/bin_packing_problem/Guided_bin_packing_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One knapsack and three items" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;Items;3\n"
      ";;Bins;3\n"
      "BinProcess;Bin1;Capacity;40\n"
      "BinProcess;Bin2;Capacity;40\n"
      "BinProcess;Bin3;Capacity;40\n"
      "ItemProcess;Item1;Size;20\n"
      "ItemProcess;Item2;Size;15\n"
      "ItemProcess;Item3;Size;22\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      readyHandler.connect(&engine);
      completionHandler.connect(&engine);

      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

    Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the knapsack considers all items" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then 2 bins are used" ) {
        auto processLog = recorder.find(nlohmann::json{{"processId", "BinProcess"},{"state", "DONE"}},nlohmann::json{{"nodeId", nullptr},{"event", nullptr},{"decision", nullptr}});
        REQUIRE( processLog.size() == 3 );
        REQUIRE( (int)processLog[0]["data"]["used"] + (int)processLog[1]["data"]["used"] + (int)processLog[2]["data"]["used"] == 2 );
      }
      THEN( "Then Item3 is allocated before Item2 which is allocated before Item1" ) {
        auto decisionLog = recorder.find(nlohmann::json{{"decision","messagedelivery"}});
        REQUIRE( decisionLog.size() == 3 );
        REQUIRE( decisionLog[0]["message"]["header"]["sender"] == "Item3" );
        REQUIRE( decisionLog[0]["evaluation"] == 18 + 2);
        REQUIRE( decisionLog[1]["message"]["header"]["sender"] == "Item2" );
        REQUIRE( decisionLog[1]["evaluation"] == 3 + 2);
        REQUIRE( decisionLog[2]["message"]["header"]["sender"] == "Item1" );
        REQUIRE( decisionLog[2]["evaluation"] == 20 + 2);
      }
      THEN( "Then Item3 is allocated before Item2 which is allocated before Item1" ) {
        auto allocationLog = recorder.find(nlohmann::json{{"nodeId", "CatchRequestMessage"},{"state", "COMPLETED"}},nlohmann::json{{"event", nullptr},{"decision", nullptr}});
        REQUIRE( allocationLog.size() == 3 );
        REQUIRE( allocationLog[0]["status"]["item"] == "Item3" );
        REQUIRE( allocationLog[1]["status"]["item"] == "Item2" );
        REQUIRE( allocationLog[2]["status"]["item"] == "Item1" );
      }
    }
  }
}
