SCENARIO( "Trivial executable process", "[execution]" ) {
  const std::string modelFile = "Execution/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();


    WHEN( "The engine is started" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&timeHandler);
      THEN( "The engine completes without error" ) {
        REQUIRE_NOTHROW( engine.run(scenario.get()) );
      }
    }
    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "The recorder log has at least one entry" ) {
        REQUIRE( recorder.log.size() >= 1 );
      }
      THEN( "The first entry of the recorder log has the correct data" ) {
        REQUIRE( recorder.log.front()["instanceId"] == "Instance_1");
        REQUIRE( recorder.log.front()["processId"] == "Process_1");
        REQUIRE( recorder.log.front()["state"] == "CREATED");
        REQUIRE( recorder.log.front()["status"]["instance"] == "Instance_1");
        REQUIRE( recorder.log.front()["status"]["timestamp"] == 0.0);
      }
    }
  }
}
