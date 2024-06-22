SCENARIO( "Task with expression operator", "[execution][task]" ) {
  const std::string modelFile = "tests/execution/task/Task_with_linear_expression.bpmn";
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
      THEN( "The token log has exactly 16 entries" ) {
        REQUIRE( tokenLog.size() == 16 );
      }
      THEN( "The dump of each entry of the token log is correct" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( activityLog[0]["state"] == "ARRIVED" );
        REQUIRE( activityLog[1]["state"] == "READY" );
        REQUIRE( activityLog[2]["state"] == "ENTERED" );
        REQUIRE( activityLog[3]["state"] == "BUSY" );
        REQUIRE( activityLog[4]["state"] == "COMPLETED" );
        REQUIRE( activityLog[5]["state"] == "EXITING" );
        REQUIRE( activityLog[6]["state"] == "DEPARTED" );

        REQUIRE( activityLog.back()["instanceId"] == "Instance_1");
        REQUIRE( activityLog.back()["processId"] == "Process_1");
        REQUIRE( activityLog.back()["state"] == "DEPARTED");
        REQUIRE( activityLog.back()["data"]["instance"] == "Instance_1");
        REQUIRE( activityLog.back()["status"]["timestamp"] == 1.0);

      }
    }
  }
}
