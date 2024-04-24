SCENARIO( "Sequential adhoc subprocess", "[execution][adhocsubprocess]" ) {
  const std::string modelFile = "execution/adhocsubprocess/AdHocSubProcess.bpmn";
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
      Execution::FirstComeFirstServedSequentialEntry sequentialEntryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      sequentialEntryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The dump of each entry of the token log is correct" ) {
        auto adHocSubProcessLog = recorder.find(nlohmann::json{{"nodeId","AdHocSubProcess_1"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( adHocSubProcessLog[0]["state"] == "ARRIVED" );
        REQUIRE( adHocSubProcessLog[1]["state"] == "READY" );
        REQUIRE( adHocSubProcessLog[2]["state"] == "ENTERED" );
        REQUIRE( adHocSubProcessLog[3]["data"]["x"] == 0 );
        REQUIRE( adHocSubProcessLog[3]["state"] == "BUSY" );
        REQUIRE( adHocSubProcessLog[4]["data"]["x"] == 1 );
        REQUIRE( adHocSubProcessLog[4]["state"] == "BUSY" );
        REQUIRE( adHocSubProcessLog[5]["data"]["x"] == 2 );
        REQUIRE( adHocSubProcessLog[5]["state"] == "BUSY" );
        REQUIRE( adHocSubProcessLog[6]["state"] == "COMPLETED" );
        REQUIRE( adHocSubProcessLog[7]["state"] == "EXITING" );
        REQUIRE( adHocSubProcessLog[8]["state"] == "DEPARTED" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( (completionLog[0]["nodeId"] == "Activity_1" || completionLog[0]["nodeId"] == "Activity_2") );
        REQUIRE( (completionLog[1]["nodeId"] == "Activity_1" || completionLog[1]["nodeId"] == "Activity_2") );
        REQUIRE( completionLog[2]["nodeId"] == "AdHocSubProcess_1" );
        REQUIRE( completionLog[3]["nodeId"] == nullptr );
      }
    }
  }
}


SCENARIO( "Sequential adhoc subprocesses with common performer", "[execution][adhocsubprocess]" ) {
  const std::string modelFile = "execution/adhocsubprocess/AdHocSubProcesses_with_common_performer.bpmn";
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
      Execution::FirstComeFirstServedSequentialEntry sequentialEntryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      sequentialEntryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The dump of each entry of the token log is correct" ) {
        auto processLog = recorder.find(nlohmann::json{},nlohmann::json{{"nodeId",nullptr},{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[0]["data"]["x"] == 0 );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["data"]["x"] == 1 );
        REQUIRE( processLog[2]["state"] == "BUSY" );
        REQUIRE( processLog[3]["data"]["x"] == 2 );
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[4]["state"] == "COMPLETED" );
        REQUIRE( processLog[5]["state"] == "DONE" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( (completionLog[0]["nodeId"] == "Activity_1" || completionLog[0]["nodeId"] == "Activity_2") );
        REQUIRE( (completionLog[1]["nodeId"] == "AdHocSubProcess_1" || completionLog[1]["nodeId"] == "AdHocSubProcess_2") );
        REQUIRE( (completionLog[2]["nodeId"] == "Activity_1" || completionLog[2]["nodeId"] == "Activity_2") );
        REQUIRE( (completionLog[3]["nodeId"] == "AdHocSubProcess_1" || completionLog[3]["nodeId"] == "AdHocSubProcess_2") );
        REQUIRE( completionLog[4]["nodeId"] == nullptr );
      }
    }
  }
}

