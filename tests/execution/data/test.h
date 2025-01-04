SCENARIO( "Task with expression operator", "[data]" ) {
  const std::string modelFile = "tests/execution/data/Data.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,DataAttribute_1,8\n"
//      "Process_1, Instance_1,DataAttribute_2,15\n"
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
      THEN( "The status updates are correct" ) {
        auto entryLog = recorder.find(nlohmann::json{{"nodeId", "SubProcess_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["data"]["data1"] == 8 );
        REQUIRE( entryLog[0]["data"]["data2"] == 0 );
        REQUIRE( entryLog[0]["data"]["data3"] == "mydata" );
        REQUIRE( entryLog[0]["data"]["data4"] == true );

        auto completionLog = recorder.find(nlohmann::json{{"nodeId", "SubProcess_1"},{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["data"]["data1"] == 42 );
        REQUIRE( completionLog[0]["data"]["data2"] == 0 );
        REQUIRE( completionLog[0]["data"]["data3"] == "mydata" );
        REQUIRE( completionLog[0]["data"]["data4"] == false );
      }
    }
  }
}
