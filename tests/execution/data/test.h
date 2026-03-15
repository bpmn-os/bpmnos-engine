SCENARIO( "Task with expression operator", "[data]" ) {
  const std::string modelFile = "tests/execution/data/Data.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with an input value" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; data1 := 8\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
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
