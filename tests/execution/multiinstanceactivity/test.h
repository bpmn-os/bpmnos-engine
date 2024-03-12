SCENARIO( "Parallel multi instance task", "[execution][multiinstanceactivity]" ) {
  const std::string modelFile = "execution/multiinstanceactivity/Parallel_multi-instance_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv,',');
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto waitingLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WAITING"}});
        REQUIRE( waitingLog[0]["status"]["timestamp"] == 0 );

        auto entryLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["status"]["timestamp"] == 0 );
        REQUIRE( entryLog[1]["status"]["timestamp"] == 0 );
        REQUIRE( entryLog[2]["status"]["timestamp"] == 0 );

        REQUIRE( entryLog[0]["status"]["counter"] == 1 );
        REQUIRE( entryLog[1]["status"]["counter"] == 2 );
        REQUIRE( entryLog[2]["status"]["counter"] == 3 );

        auto exitLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "EXITING"}});
        REQUIRE( exitLog[0]["status"]["timestamp"] == 3 );
        REQUIRE( exitLog[1]["status"]["timestamp"] == 3 );
        REQUIRE( exitLog[2]["status"]["timestamp"] == 3 );

        auto departureLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DEPARTED"}});
        REQUIRE( departureLog[0]["status"]["timestamp"] == 3 );
      }
    }
  }
}

SCENARIO( "Sequential multi instance task", "[execution][multiinstanceactivity]" ) {
  const std::string modelFile = "execution/multiinstanceactivity/Sequential_multi-instance_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv,',');
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto waitingLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WAITING"}});
        REQUIRE( waitingLog[0]["status"]["timestamp"] == 0 );

        auto entryLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["status"]["timestamp"] == 0 );
        REQUIRE( entryLog[1]["status"]["timestamp"] == 1 );
        REQUIRE( entryLog[2]["status"]["timestamp"] == 2 );

        REQUIRE( entryLog[0]["status"]["counter"] == 1 );
        REQUIRE( entryLog[1]["status"]["counter"] == 2 );
        REQUIRE( entryLog[2]["status"]["counter"] == 3 );

        auto exitLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "EXITING"}});
        REQUIRE( exitLog[0]["status"]["timestamp"] == 1 );
        REQUIRE( exitLog[1]["status"]["timestamp"] == 2 );
        REQUIRE( exitLog[2]["status"]["timestamp"] == 3 );

        auto departureLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DEPARTED"}});
        REQUIRE( departureLog[0]["status"]["timestamp"] == 3 );
      }
    }
  }
}

SCENARIO( "Parallel multi instance task with timeout", "[execution][multiinstanceactivity]" ) {
  const std::string modelFile = "execution/multiinstanceactivity/Parallel_multi-instance_task_with_timeout.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv,',');
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto waitingLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WAITING"}});
        REQUIRE( waitingLog[0]["status"]["timestamp"] == 0 );

        auto entryLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["status"]["timestamp"] == 0 );
        REQUIRE( entryLog[1]["status"]["timestamp"] == 0 );
        REQUIRE( entryLog[2]["status"]["timestamp"] == 0 );

        REQUIRE( entryLog[0]["status"]["counter"] == 1 );
        REQUIRE( entryLog[1]["status"]["counter"] == 2 );
        REQUIRE( entryLog[2]["status"]["counter"] == 3 );

        auto withdrawnLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog[0]["status"]["timestamp"] >= 2 );
        REQUIRE( withdrawnLog[1]["status"]["timestamp"] >= 2 );
        REQUIRE( withdrawnLog[2]["status"]["timestamp"] >= 2 );

        auto exitLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "EXITING"}});
        REQUIRE( exitLog.size() == 0 );

        auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state", "COMPLETED"}});
        REQUIRE( timerLog.size() == 1 );
        REQUIRE( timerLog[0]["status"]["timestamp"] == 2 );


        auto departureLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DEPARTED"}});
        REQUIRE( departureLog.size() == 0 );
      }
    }
  }
}

SCENARIO( "Sequential multi instance task with timeout", "[execution][multiinstanceactivity]" ) {
  const std::string modelFile = "execution/multiinstanceactivity/Sequential_multi-instance_task_with_timeout.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv,',');
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto waitingLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WAITING"}});
        REQUIRE( waitingLog[0]["status"]["timestamp"] == 0 );

        auto entryLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["status"]["timestamp"] == 0 );
        REQUIRE( entryLog[1]["status"]["timestamp"] == 1 );
        REQUIRE( entryLog.size() < 3 );

        REQUIRE( entryLog[0]["status"]["counter"] == 1 );
        REQUIRE( entryLog[1]["status"]["counter"] == 2 );

        auto withdrawnLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog.size() >= 1 );
        REQUIRE( withdrawnLog[0]["status"]["timestamp"] == 2 );

        auto exitLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "EXITING"}});
        REQUIRE( exitLog.size() >= 1 );
        REQUIRE( exitLog.size() < 3 );

        auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state", "COMPLETED"}});
        REQUIRE( timerLog.size() == 1 );
        REQUIRE( timerLog[0]["status"]["timestamp"] == 2 );

        auto departureLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DEPARTED"}});
        REQUIRE( departureLog.size() == 0 );
      }
    }
  }
}

SCENARIO( "Sequential multi instance subprocess with error", "[execution][multiinstanceactivity]" ) {
  const std::string modelFile = "execution/multiinstanceactivity/Sequential_multi-instance_subprocess_with_error.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv,',');
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto waitingLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WAITING"}});
        REQUIRE( waitingLog.size() == 1 );

        auto entryLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 1 );

        REQUIRE( entryLog[0]["status"]["counter"] == 1 );

        auto withdrawnLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog.size() == 3 );

        auto exitLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "EXITING"}});
        REQUIRE( exitLog.size() == 0 );

        auto errorLog = recorder.find(nlohmann::json{{"nodeId","ErrorBoundaryEvent_1"},{"state", "COMPLETED"}});
        REQUIRE( errorLog.size() == 1 );

        auto departureLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DEPARTED"}});
        REQUIRE( departureLog.size() == 0 );
      }
    }
  }
}

SCENARIO( "Sequential multi instance subprocess with escalation", "[execution][multiinstanceactivity]" ) {
  const std::string modelFile = "execution/multiinstanceactivity/Sequential_multi-instance_subprocess_with_escalation.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv,',');
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto waitingLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WAITING"}});
        REQUIRE( waitingLog.size() == 1 );

        auto entryLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 3 );

        auto withdrawnLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog.size() == 0 );

        auto exitLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "EXITING"}});
        REQUIRE( exitLog.size() == 3 );

        auto escalationLog = recorder.find(nlohmann::json{{"nodeId","EscalationBoundaryEvent_1"},{"state", "COMPLETED"}});
        REQUIRE( escalationLog.size() == 3 );

        auto departureLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DEPARTED"}});
        REQUIRE( departureLog.size() == 1 );
      }
    }
  }
}
