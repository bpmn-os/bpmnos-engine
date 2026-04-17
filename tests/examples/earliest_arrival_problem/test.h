SCENARIO( "Earliest arrival problem", "[examples][earliest_arrival_problem]" ) {
  const std::string modelFile = "examples/earliest_arrival_problem/Earliest_arrival_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/earliest_arrival_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "An earliest arrival problem" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance1; EarliestArrival_Process; initial_location := \"Origin\"\n"
      "Instance1; EarliestArrival_Process; final_location := \"Destination\"\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,folders,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the greedy controller" ) {
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
      THEN( "Then locations are visited in the heuristic order" ) {
        auto travelLog = recorder.find(nlohmann::json{{"nodeId", "Travel"},{"state", "COMPLETED"}});
        REQUIRE( travelLog[0]["status"]["current_location"] == "Location1" );
        REQUIRE( travelLog[1]["status"]["current_location"] == "Destination" );
      }
    }
  }
}

