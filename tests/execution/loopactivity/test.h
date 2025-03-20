SCENARIO( "Loop task", "[execution][loopactivity]" ) {
  const std::string modelFile = "tests/execution/loopactivity/Loop_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {


    WHEN( "The engine is started with a maximum of 2 loops" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1, Maximum, 2\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

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
      THEN( "The loop activity is performed twice" ) {
        auto entryLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 2 );
 
        auto exitLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "EXITING"}});
        REQUIRE( exitLog.size() == 2 );
      }
    }

    WHEN( "The engine is started with an irrelevant loop maximum" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1, Maximum, 4\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

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
      THEN( "The loop activity is performed as long as the loop condition holds" ) {
        auto entryLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 3 );
 
        auto exitLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "EXITING"}});
        REQUIRE( exitLog.size() == 3 );
      }
    }
  }
}


