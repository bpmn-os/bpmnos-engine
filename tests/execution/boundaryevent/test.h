SCENARIO( "Failed task", "[execution][boundaryevent]" ) {
  const std::string modelFile = "execution/boundaryevent/Failed_Task.bpmn";
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
      engine.run(scenario.get(),1);
      THEN( "The dump of the recorder log is correct" ) {

        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "BoundaryEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "EndEvent_1" );

        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog[0]["nodeId"] == "Activity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "BoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == nullptr );


        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );
        REQUIRE( processLog[3]["status"]["timestamp"] == 1.0 );
      }
    }
  }
}

SCENARIO( "Failed subprocess", "[execution][boundaryevent]" ) {
  const std::string modelFile = "execution/boundaryevent/Failed_SubProcess.bpmn";
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
      engine.run(scenario.get(),0);
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog[0]["nodeId"] == "ErrorEndEvent_1" );
        REQUIRE( failureLog[1]["nodeId"] == "Activity_1" );

        auto withdrawnLog = recorder.find(nlohmann::json{{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog[0]["nodeId"] == "BoundaryEvent_2" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "BoundaryEvent_2" );
        REQUIRE( completionLog[1]["nodeId"] == "BoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == nullptr );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );
      }
    }
  }
}

