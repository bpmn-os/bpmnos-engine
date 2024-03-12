SCENARIO( "Simple process with timer", "[execution][timer]" ) {
  const std::string modelFile = "execution/timer/Timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started without input values" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,,\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv,',');
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The timer is triggered at the default time" ) {
        auto timerLog =recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state", "COMPLETED"}});
        REQUIRE( timerLog.front()["status"]["timestamp"] == 5.0 ); 
      }
    }

    WHEN( "The engine is started with a given trigger" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Trigger,10\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv,',');
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      timeHandler.subscribe(&engine);
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
