SCENARIO( "Knapsack problem", "[examples][knapsack_problem]" ) {
  const std::string modelFile = "examples/knapsack_problem/Knapsack_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One knapsack and three items" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "KnapsackProcess,Knapsack1,Capacity,40\n"
      "ItemProcess,Item1,Weight,20\n"
      "ItemProcess,Item1,Value,100\n"
      "ItemProcess,Item2,Weight,15\n"
      "ItemProcess,Item2,Value,50\n"
      "ItemProcess,Item3,Weight,22\n"
      "ItemProcess,Item3,Value,120\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntryHandler parallelEntryHandler;
      Execution::NaiveSequentialEntryHandler sequentialEntryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::FirstMatchingMessageHandler messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&messageHandler);
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&parallelEntryHandler);
      engine.addEventHandler(&sequentialEntryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&messageTaskTerminator);
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "Some items are accepted, some are rejected" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ItemRejected"}}).size() > 0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ItemAccepted"}}).size() > 0);
      }
    }
  }
}