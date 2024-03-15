SCENARIO( "Compensate throw event without compensations", "[execution][compensation]" ) {
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
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateThrowEvent_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Task with unused compensation task", "[execution][compensation]" ) {
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

SCENARIO( "Task with compensation task", "[execution][compensation]" ) {
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

SCENARIO( "Task with compensation triggered by error", "[execution][compensation]" ) {
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

SCENARIO( "Task with compensation subprocess", "[execution][compensation]" ) {
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

SCENARIO( "Named task with compensation task", "[execution][compensation]" ) {
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

SCENARIO( "Multi instance compensation", "[execution][compensation][multiinstanceactivity]" ) {
  const std::string modelFile = "execution/compensationactivity/Multi-instance_compensation.bpmn";
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto waitingLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WAITING"}});
        REQUIRE( waitingLog.size() == 1 );

        auto entryLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 3 );

        auto withdrawnLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog.size() == 1 );

        auto exitLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "EXITING"}});
        REQUIRE( exitLog.size() == 3 );

        auto compensationLog = recorder.find(nlohmann::json{{"nodeId","CompensateBoundaryEvent_1"},{"state", "COMPLETED"}});
        REQUIRE( compensationLog.size() == 2 );

        auto compensationActivityLog = recorder.find(nlohmann::json{{"nodeId","CompensationActivity_1"},{"state", "COMPLETED"}});
        REQUIRE( compensationActivityLog.size() == 2 );

        auto departureLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DEPARTED"}});
        REQUIRE( departureLog.size() == 0 );
      }
    }
  }
}


SCENARIO( "Compensation of  multi instance activity", "[execution][compensation][multiinstanceactivity]" ) {
  const std::string modelFile = "execution/compensationactivity/Compensation_multi-instance_activity.bpmn";
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
//      engine.run(scenario.get());
      THEN( "The engine throws an error" ) {
        REQUIRE_THROWS( engine.run(scenario.get()) );
      }
    }
  }
}

SCENARIO( "Failing compensations of  multi instance activity", "[execution][compensation][multiinstanceactivity]" ) {
  const std::string modelFile = "execution/compensationactivity/Failing_compensations_multi-instance_activity.bpmn";
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
//        REQUIRE_THROWS( engine.run(scenario.get()) );
        auto compensationLog = recorder.find(nlohmann::json{{"nodeId","CompensationActivity_1"}});
        REQUIRE( compensationLog[0]["state"] == "ENTERED" );
        REQUIRE( compensationLog[1]["state"] == "BUSY" );
        REQUIRE( compensationLog[2]["state"] == "FAILED" );

        REQUIRE( compensationLog.size() == 3 );
        
        auto failureLog = recorder.find(nlohmann::json{{"state","FAILED"}});
        REQUIRE( failureLog[0]["nodeId"] == "ErrorEvent_1" );
        REQUIRE( failureLog[1]["nodeId"] == "ErrorEvent_2" );
        REQUIRE( failureLog[2]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( failureLog[3]["nodeId"] == nullptr );
        
      }
    }
  }
}

