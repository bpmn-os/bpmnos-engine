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
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::FirstComeFirstServedSequentialEntryHandler sequentialEntryHandler;
      Execution::FirstMatchingMessageHandler messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      sequentialEntryHandler.subscribe(&engine);
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
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
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::BestFirstSequentialEntryHandler sequentialEntryHandler;
      Execution::FirstMatchingMessageHandler messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      sequentialEntryHandler.subscribe(&engine);
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
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
