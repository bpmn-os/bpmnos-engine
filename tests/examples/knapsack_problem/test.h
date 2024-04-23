SCENARIO( "Knapsack problem", "[examples][knapsack_problem]" ) {
  const std::string modelFile = "examples/knapsack_problem/Knapsack_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One knapsack and three items" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "KnapsackProcess;Knapsack1;Capacity;40\n"
      "ItemProcess;Item1;Weight;20\n"
      "ItemProcess;Item1;Value;100\n"
      "ItemProcess;Item2;Weight;15\n"
      "ItemProcess;Item2;Value;50\n"
      "ItemProcess;Item3;Weight;22\n"
      "ItemProcess;Item3;Value;120\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with naive dispatcher" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry parallelEntryHandler;
      Execution::FirstComeFirstServedSequentialEntry sequentialEntryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.connect(&engine);
      readyHandler.connect(&engine);
      parallelEntryHandler.connect(&engine);
      sequentialEntryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Some items are accepted, some are rejected" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ItemRejected"}}).size() > 0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ItemAccepted"}}).size() > 0);
      }
    }

    WHEN( "The engine is started with the guided controller (but no guidance)" ) {
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
      THEN( "Then the knapsack is closed before items are included" ) {
        auto failureLog = recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 3 );
      }
    }
  }
}

SCENARIO( "Guided knapsack problem", "[examples][knapsack_problem]" ) {
  const std::string modelFile = "examples/knapsack_problem/Guided_knapsack_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One knapsack and three items" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "KnapsackProcess;Knapsack1;Items;3\n"
      "KnapsackProcess;Knapsack1;Capacity;40\n"
      "ItemProcess;Item1;Weight;20\n"
      "ItemProcess;Item1;Value;100\n"
      "ItemProcess;Item2;Weight;15\n"
      "ItemProcess;Item2;Value;50\n"
      "ItemProcess;Item3;Weight;22\n"
      "ItemProcess;Item3;Value;120\n"
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
        auto failureLog = recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then the knapsack handles items with best value to weight ratio first" ) {
        auto handleLog = recorder.find(nlohmann::json{{"nodeId", "HandleItemActivity"},{"state", "COMPLETED"}});
        REQUIRE( handleLog[0]["status"]["item"] == "Item3" );
        REQUIRE( handleLog[1]["status"]["item"] == "Item1" );
        REQUIRE( handleLog[2]["status"]["item"] == "Item2" );
      }
      THEN( "Then the knapsack includes Item3 and Item2" ) {
        auto acceptanceLog = recorder.find(nlohmann::json{{"nodeId", "ItemAccepted"},{"state", "ENTERED"}});
        REQUIRE( (acceptanceLog[0]["instanceId"] == "Item2" || acceptanceLog[0]["instanceId"] == "Item3") );
        REQUIRE( (acceptanceLog[1]["instanceId"] == "Item2" || acceptanceLog[1]["instanceId"] == "Item3") );
      }
    }
  }
}
