SCENARIO( "Parallel multi instance task", "[execution][multiinstanceactivity]" ) {
  const std::string modelFile = "tests/execution/multiinstanceactivity/Parallel_multi-instance_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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

        // each instance is born (CREATED), gets its own entry decision (READY), and ends at DONE;
        // the main token neither emits READY nor DONE (it departs)
        auto createdLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "CREATED"}});
        REQUIRE( createdLog.size() == 3 );
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "READY"}});
        REQUIRE( readyLog.size() == 3 );
        auto doneLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DONE"}});
        REQUIRE( doneLog.size() == 3 );

        auto departureLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DEPARTED"}});
        REQUIRE( departureLog[0]["status"]["timestamp"] == 3 );
      }
    }
  }
}

SCENARIO( "Sequential multi instance task", "[execution][multiinstanceactivity]" ) {
  const std::string modelFile = "tests/execution/multiinstanceactivity/Sequential_multi-instance_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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

        // each instance is born (CREATED), gets its own entry decision (READY), and ends at DONE.
        // Before the fix, sequential instances #2/#3 emitted no READY and the main token emitted a
        // spurious one; READY.size() == 3 pins both directions of that regression.
        auto createdLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "CREATED"}});
        REQUIRE( createdLog.size() == 3 );
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "READY"}});
        REQUIRE( readyLog.size() == 3 );
        REQUIRE( readyLog[0]["status"]["counter"] == 1 );
        REQUIRE( readyLog[1]["status"]["counter"] == 2 );
        REQUIRE( readyLog[2]["status"]["counter"] == 3 );
        auto doneLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DONE"}});
        REQUIRE( doneLog.size() == 3 );

        auto departureLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DEPARTED"}});
        REQUIRE( departureLog[0]["status"]["timestamp"] == 3 );
      }
    }
  }
}


SCENARIO( "Parallel multi instance task with timeout", "[execution][multiinstanceactivity]" ) {
  const std::string modelFile = "tests/execution/multiinstanceactivity/Parallel_multi-instance_task_with_timeout.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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

        // all instances are born and become ready before the timeout withdraws them
        auto createdLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "CREATED"}});
        REQUIRE( createdLog.size() == 3 );
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "READY"}});
        REQUIRE( readyLog.size() == 3 );

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
  const std::string modelFile = "tests/execution/multiinstanceactivity/Sequential_multi-instance_task_with_timeout.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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

        // all instances are born up front even though the timeout stops the sequence early;
        // at least the first two reach their own READY state
        auto createdLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "CREATED"}});
        REQUIRE( createdLog.size() == 3 );
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "READY"}});
        REQUIRE( readyLog.size() >= 2 );

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
  const std::string modelFile = "tests/execution/multiinstanceactivity/Sequential_multi-instance_subprocess_with_error.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto waitingLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WAITING"}});
        REQUIRE( waitingLog.size() == 1 );

        auto entryLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 1 );

        REQUIRE( entryLog[0]["status"]["counter"] == 1 );

        auto withdrawnLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog.size() == 3 );

        // all instances are born even though only the first enters before the error aborts the rest
        auto createdLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "CREATED"}});
        REQUIRE( createdLog.size() == 3 );

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
  const std::string modelFile = "tests/execution/multiinstanceactivity/Sequential_multi-instance_subprocess_with_escalation.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto waitingLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WAITING"}});
        REQUIRE( waitingLog.size() == 1 );

        auto entryLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 3 );

        auto withdrawnLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog.size() == 0 );

        // all instances are born, become ready, and reach DONE on exit (escalation is non-interrupting)
        auto createdLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "CREATED"}});
        REQUIRE( createdLog.size() == 3 );
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "READY"}});
        REQUIRE( readyLog.size() == 3 );
        auto doneLog = recorder.find(nlohmann::json{{"nodeId","MultiInstanceActivity_1"},{"state", "DONE"}});
        REQUIRE( doneLog.size() == 3 );

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

