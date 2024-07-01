SCENARIO( "Decision task with enumeration", "[execution][decisiontask]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn";
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
      Execution::LocalEvaluator evaluator;
      Execution::BestLimitedChoice choiceHandler(&evaluator);
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      choiceHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);

      engine.run(scenario.get());

      THEN( "The dump of each entry of the token log is correct" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" },{"state","COMPLETED"}});
        REQUIRE( activityLog[0]["status"]["choice"] == 1 );
      }
    }
  }
}

SCENARIO( "Decision task with bounds", "[execution][decisiontask]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_bounds.bpmn";
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
      Execution::LocalEvaluator evaluator;
      Execution::BestLimitedChoice choiceHandler(&evaluator);
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      choiceHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);

      engine.run(scenario.get());

      THEN( "The dump of each entry of the token log is correct" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" },{"state","COMPLETED"}});
        REQUIRE( activityLog[0]["status"]["choice"] == -2 );
      }
    }
  }
}
