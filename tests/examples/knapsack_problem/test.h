SCENARIO( "Knapsack problem", "[examples][knapsack_problem]" ) {
  const std::string modelFile = "examples/knapsack_problem/Knapsack_problem.bpmn";
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

    WHEN( "The engine is started with naive dispatcher" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry parallelEntryHandler;
//      Execution::FirstComeFirstServedSequentialEntry sequentialEntryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.connect(&engine);
      readyHandler.connect(&engine);
      parallelEntryHandler.connect(&engine);
//      sequentialEntryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The run terminates without failure and some items are accepted, some are rejected" ) {
        auto failureLog = recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ItemRejected"}}).size() > 0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ItemAccepted"}}).size() > 0);
      }
    }
  }
}

