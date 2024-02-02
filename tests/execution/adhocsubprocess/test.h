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
      Execution::InstantEntryHandler entryHandler;
      Execution::NaiveSequentialEntryHandler sequentialEntryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&entryHandler);
      engine.addEventHandler(&sequentialEntryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto adHocSubProcessLog = recorder.find(nlohmann::json{{"nodeId","AdHocSubProcess_1"}});
        REQUIRE( adHocSubProcessLog[0]["state"] == "ARRIVED" );
        REQUIRE( adHocSubProcessLog[1]["state"] == "READY" );
        REQUIRE( adHocSubProcessLog[2]["state"] == "ENTERED" );
        REQUIRE( adHocSubProcessLog[3]["status"]["x"] == 0 );
        REQUIRE( adHocSubProcessLog[3]["state"] == "IDLE" );
        REQUIRE( adHocSubProcessLog[4]["status"]["x"] == 0 );
        REQUIRE( adHocSubProcessLog[4]["state"] == "BUSY" );
        REQUIRE( adHocSubProcessLog[5]["status"]["x"] == 1 );
        REQUIRE( adHocSubProcessLog[5]["state"] == "IDLE" );
        REQUIRE( adHocSubProcessLog[6]["status"]["x"] == 1 );
        REQUIRE( adHocSubProcessLog[6]["state"] == "BUSY" );
        REQUIRE( adHocSubProcessLog[7]["status"]["x"] == 2 );
        REQUIRE( adHocSubProcessLog[7]["state"] == "IDLE" );
        REQUIRE( adHocSubProcessLog[8]["state"] == "COMPLETED" );
        REQUIRE( adHocSubProcessLog[9]["state"] == "EXITING" );
        REQUIRE( adHocSubProcessLog[10]["state"] == "DEPARTED" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( (completionLog[0]["nodeId"] == "Activity_1" || completionLog[0]["nodeId"] == "Activity_2") );
        REQUIRE( (completionLog[1]["nodeId"] == "Activity_1" || completionLog[1]["nodeId"] == "Activity_2") );
        REQUIRE( completionLog[2]["nodeId"] == "AdHocSubProcess_1" );
        REQUIRE( completionLog[3]["nodeId"] == nullptr );
      }
    }
  }
}

