SCENARIO( "A simple process with subprocess and task", "[collection][process]" ) {
  const std::string modelFile = "tests/execution/collection/Collection.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The attribute values are properly initialized" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( entryLog[0]["processId"] == "Process_1" );
        REQUIRE( entryLog[0]["status"]["x"] == encodeQuotedStrings(R"([ "A", "B", "C" ])") );
        REQUIRE( entryLog[0]["status"]["i"] == 2 );
        REQUIRE( entryLog[0]["status"]["z"] == "B" );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( entryLog[2]["status"]["v"] == "A" );
        REQUIRE( entryLog[3]["nodeId"] == "Task_1" );
        REQUIRE( entryLog[3]["status"]["w"] == "C" );

        auto readyLog = recorder.find(nlohmann::json{{"state", "READY"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( readyLog[0]["nodeId"] == "SubProcess_1" );
        REQUIRE( readyLog[0]["status"]["v"] == "A" );
        REQUIRE( readyLog[1]["nodeId"] == "Task_1" );
        REQUIRE( readyLog[1]["status"]["w"] == "C" );
      }
    }
  }
}
