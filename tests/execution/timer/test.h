SCENARIO( "Simple process with timer", "[execution][timer]" ) {
  const std::string modelFile = "tests/execution/timer/Timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started with a given trigger" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; trigger := 10\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

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
      THEN( "The timer is triggered at the given time" ) {
        auto timerLog =recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state", "COMPLETED"}});
        REQUIRE( timerLog.front()["status"]["timestamp"] == 10.0 ); 
      }
    }
  }
}
