SCENARIO( "Travelling salesperson problem", "[examples][travelling_salesperson_problem]" ) {
  const std::string modelFile = "examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn";
  BPMNOS::Model::LookupTable::folders = { std::string(std::filesystem::current_path()) + "/examples/travelling_salesperson_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A TSP with four location" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "TravellingSalesperson_Process;Instance1;Origin;Hamburg\n"
      "TravellingSalesperson_Process;Instance1;Locations;[\"Munich\",\"Berlin\",\"Cologne\"]\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a first-come-first-serve policy" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstComeFirstServedSequentialEntry sequentialEntryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      sequentialEntryHandler.connect(&engine);
      messageHandler.connect(&engine);
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then locations are visited in the given order" ) {
        auto visitLog = recorder.find(nlohmann::json{{"nodeId", "VisitLocation"},{"state", "ENTERED"}});
        REQUIRE( visitLog[0]["status"]["location"] == "Munich" );
        REQUIRE( visitLog[1]["status"]["location"] == "Berlin" );
        REQUIRE( visitLog[2]["status"]["location"] == "Cologne" );
      }
    }

    WHEN( "The engine is started with a best-first policy" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::BestFirstSequentialEntry sequentialEntryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      sequentialEntryHandler.connect(&engine);
      messageHandler.connect(&engine);
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then locations are visited in the nearest-neighbour order" ) {
        auto visitLog = recorder.find(nlohmann::json{{"nodeId", "VisitLocation"},{"state", "ENTERED"}});
        REQUIRE( visitLog[0]["status"]["location"] == "Berlin" );
        REQUIRE( visitLog[1]["status"]["location"] == "Cologne" );
        REQUIRE( visitLog[2]["status"]["location"] == "Munich" );
      }
    }
  }

}
