SCENARIO( "Compensate throw event without compensations", "[compensationactivity][throwevent]" ) {
  const std::string modelFile = "execution/compensationactivity/No_compensation.bpmn";
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
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&entryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateThrowEvent_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Task with unused compensation task", "[compensationactivity][task]" ) {
  const std::string modelFile = "execution/compensationactivity/Unused_compensation_task.bpmn";
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
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&entryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "EndEvent_1" );

        auto busyLog = recorder.find(nlohmann::json{{"state", "BUSY"}});
        REQUIRE( busyLog[0]["nodeId"] == nullptr );
        REQUIRE( busyLog[1]["nodeId"] == "Activity_1" );
        REQUIRE( busyLog[2]["nodeId"] == "CompensateBoundaryEvent_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Task with compensation task", "[compensationactivity][task]" ) {
  const std::string modelFile = "execution/compensationactivity/Compensation_task.bpmn";
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
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&entryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Task with compensation triggered by error", "[compensationactivity][error]" ) {
  const std::string modelFile = "execution/compensationactivity/Compensation_triggered_by_error.bpmn";
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
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&entryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "ErrorEndEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensationActivity_1" );

        auto failedLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failedLog[0]["nodeId"] == "ErrorEndEvent_1" );
        REQUIRE( failedLog[1]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Task with compensation task", "[compensationactivity][subprocess]" ) {
  const std::string modelFile = "execution/compensationactivity/Compensation_subprocess.bpmn";
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
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&entryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Named task with compensation task", "[compensationactivity][named]" ) {
  const std::string modelFile = "execution/compensationactivity/Named_compensation.bpmn";
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
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&entryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == nullptr );
     }
    }
  }
}

