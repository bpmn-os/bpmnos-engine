SCENARIO( "Intermediate escalation", "[execution][escalation]" ) {
  const std::string modelFile = "tests/execution/escalationevent/Uncaught_escalation.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

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
      auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
      THEN( "The token log has exactly 20 entries" ) {
        REQUIRE( tokenLog.size() == 20 );
      }
      THEN( "The dump of each entry of the token log is correct" ) {
        auto escalationEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationEventLog[0]["status"]["timestamp"] == 1.0);
        REQUIRE( escalationEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEventLog[1]["state"] == "ENTERED" );
        REQUIRE( escalationEventLog[2]["state"] == "DEPARTED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[1]["status"]["timestamp"] == 0.0);
        REQUIRE( processLog[2]["state"] == "BUSY" );
        REQUIRE( processLog[2]["status"]["timestamp"] == 1.0);
        REQUIRE( processLog[3]["state"] == "COMPLETED" );
        REQUIRE( processLog[4]["state"] == "DONE" );
      }
    }
  }
}
