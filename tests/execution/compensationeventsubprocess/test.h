SCENARIO( "Simple compensation event subprocess", "[execution][compensation]" ) {
  const std::string modelFile = "execution/compensationeventsubprocess/Simple_compensation_event_subprocess.bpmn";
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
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( entryLog[3]["nodeId"] == "StartEvent_2" );
        REQUIRE( entryLog[4]["nodeId"] == "EndEvent_2" );
        REQUIRE( entryLog[5]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[6]["nodeId"] == "EndEvent_3" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "SubProcess_1" );
        REQUIRE( completionLog[1]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[3]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Recursive compensations", "[execution][compensation]" ) {
  const std::string modelFile = "execution/compensationeventsubprocess/Recursive_compensations.bpmn";
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
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( entryLog[3]["nodeId"] == "StartEvent_2" );
        REQUIRE( entryLog[4]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[5]["nodeId"] == "Activity_2" );
        REQUIRE( entryLog[6]["nodeId"] == "EndEvent_2" );
        REQUIRE( entryLog[7]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[8]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( entryLog[9]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( entryLog[10]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "Activity_2" );
        REQUIRE( completionLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == "CompensateBoundaryEvent_2" );
        REQUIRE( completionLog[5]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( completionLog[6]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[7]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[8]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( completionLog[9]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[10]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Recursive named compensations", "[execution][compensation]" ) {
  const std::string modelFile = "execution/compensationeventsubprocess/Recursive_named_compensations.bpmn";
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
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( entryLog[3]["nodeId"] == "StartEvent_2" );
        REQUIRE( entryLog[4]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[5]["nodeId"] == "Activity_2" );
        REQUIRE( entryLog[6]["nodeId"] == "EndEvent_2" );
        REQUIRE( entryLog[7]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[8]["nodeId"] == "CompensateThrowEvent_2" );
        REQUIRE( entryLog[9]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( entryLog[10]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( entryLog[11]["nodeId"] == "CompensationActivity_2" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "Activity_2" );
        REQUIRE( completionLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[5]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[6]["nodeId"] == "CompensateThrowEvent_2" );
        REQUIRE( completionLog[7]["nodeId"] == "CompensateBoundaryEvent_2" );
        REQUIRE( completionLog[8]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( completionLog[9]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( completionLog[10]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[11]["nodeId"] == nullptr );
     }
    }
  }
}

