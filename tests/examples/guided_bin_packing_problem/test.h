SCENARIO( "Guided bin packing problem", "[examples][bin_packing_problem]" ) {
  const std::string modelFile = "examples/guided_bin_packing_problem/Guided_bin_packing_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Three bins and three items" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; bins := 3\n"
      "; ; items := 3\n"
      "Bin1; BinProcess; capacity := 40.0\n"
      "Bin2; BinProcess; capacity := 40.0\n"
      "Bin3; BinProcess; capacity := 40.0\n"
      "Item1; ItemProcess; size := 20.0\n"
      "Item2; ItemProcess; size := 15.0\n"
      "Item3; ItemProcess; size := 22.0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;

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
      THEN( "Then no failure occurs" ) {
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
        REQUIRE( decisionLog[0]["message"]["header"]["sender"] == std::to_string(stringRegistry("Item3")) );
        REQUIRE( decisionLog[0]["evaluation"] == 18);
        REQUIRE( decisionLog[1]["message"]["header"]["sender"] == std::to_string(stringRegistry("Item2")) );
        REQUIRE( decisionLog[1]["evaluation"] == 3);
        REQUIRE( decisionLog[2]["message"]["header"]["sender"] == std::to_string(stringRegistry("Item1")) );
        REQUIRE( decisionLog[2]["evaluation"] == 20);
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

  GIVEN( "Four bins and four items" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; bins := 4\n"
      "; ; items := 4\n"
      "Bin1; BinProcess; capacity := 100.0\n"
      "Bin2; BinProcess; capacity := 100.0\n"
      "Bin3; BinProcess; capacity := 100.0\n"
      "Bin4; BinProcess; capacity := 100.0\n"
      "Item1; ItemProcess; size := 36.6\n"
      "Item2; ItemProcess; size := 26.8\n"
      "Item3; ItemProcess; size := 36.6\n"
      "Item4; ItemProcess; size := 43.0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;

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
      THEN( "Then no failure occurs" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
        auto processLog = recorder.find(nlohmann::json{{"processId", "BinProcess"},{"state", "DONE"}},nlohmann::json{{"nodeId", nullptr},{"event", nullptr},{"decision", nullptr}});
        REQUIRE( processLog.size() == 4 );
      }
    }
  }
}

