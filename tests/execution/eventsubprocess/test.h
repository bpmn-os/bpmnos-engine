SCENARIO( "Caught error end event", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "tests/execution/eventsubprocess/Caught_error.bpmn";
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
      engine.run(scenario.get(),0);
      THEN( "The dump of each entry of the token log is correct" ) {

        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto errorStartEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorStartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( errorStartEventLog[0]["state"] == "ENTERED" );
        REQUIRE( errorStartEventLog[1]["state"] == "BUSY" );
        REQUIRE( errorStartEventLog[2]["state"] == "COMPLETED" );
        REQUIRE( errorStartEventLog[3]["state"] == "EXITING" );
        REQUIRE( errorStartEventLog[4]["state"] == "DONE" );

        auto errorEndEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( errorEndEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEventLog[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEventLog[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );

      }
    }
  }
}

SCENARIO( "Interrupting escalation", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "tests/execution/eventsubprocess/Interrupting_escalation.bpmn";
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
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto escalationStartEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationStartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationStartEventLog[0]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog[0]["state"] == "ENTERED" );
        REQUIRE( escalationStartEventLog[1]["state"] == "BUSY" );
        REQUIRE( escalationStartEventLog[2]["status"]["timestamp"] == 1.0);
        REQUIRE( escalationStartEventLog[2]["state"] == "COMPLETED" );
        REQUIRE( escalationStartEventLog[3]["state"] == "EXITING" );
        REQUIRE( escalationStartEventLog[4]["state"] == "DONE" );

        auto escalationEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEventLog[1]["state"] == "ENTERED" );
        REQUIRE( escalationEventLog[2]["state"] == "WITHDRAWN" );

        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( activityLog[0]["state"] == "ARRIVED" );
        REQUIRE( activityLog[1]["state"] == "READY" );
        REQUIRE( activityLog[2]["state"] == "ENTERED" );
        REQUIRE( activityLog[3]["state"] == "BUSY" );
        REQUIRE( activityLog[4]["state"] == "COMPLETED" );
        REQUIRE( activityLog[5]["state"] == "EXITING" );
        REQUIRE( activityLog[6]["state"] == "DEPARTED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );
      }
    }
  }
}

SCENARIO( "Non-interrupting escalation", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "tests/execution/eventsubprocess/Non-interrupting_escalation.bpmn";
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
      THEN( "The dump of each entry of the token log is correct" ) {
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto escalationStartEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationStartEvent_1" },{"state","ENTERED"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationStartEventLog[0]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog[0]["data"]["instance"] == "Instance_1^EventSubProcess_1#1");
        REQUIRE( escalationStartEventLog[1]["status"]["timestamp"] == 1.0);
        REQUIRE( escalationStartEventLog[1]["data"]["instance"] == "Instance_1^EventSubProcess_1#2");
        
        auto escalationEvent1Log = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationEvent1Log[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEvent1Log[1]["state"] == "ENTERED" );
        REQUIRE( escalationEvent1Log[2]["state"] == "DEPARTED" );

        auto escalationEvent2Log = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_2" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationEvent2Log[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEvent2Log[1]["state"] == "ENTERED" );
        REQUIRE( escalationEvent2Log[2]["state"] == "DEPARTED" );

        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( activityLog[0]["state"] == "ARRIVED" );
        REQUIRE( activityLog[1]["state"] == "READY" );
        REQUIRE( activityLog[2]["state"] == "ENTERED" );
        REQUIRE( activityLog[3]["state"] == "BUSY" );
        REQUIRE( activityLog[4]["state"] == "COMPLETED" );
        REQUIRE( activityLog[5]["state"] == "EXITING" );
        REQUIRE( activityLog[6]["state"] == "DEPARTED" );
        
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );

      }
    }
  }
}

SCENARIO( "Caught and rethrown error", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "tests/execution/eventsubprocess/Caught_and_rethrown_error.bpmn";
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
      engine.run(scenario.get(),0);
      THEN( "The dump of each entry of the token log is correct" ) {
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto errorStartEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorStartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( errorStartEventLog[0]["state"] == "ENTERED" );
        REQUIRE( errorStartEventLog[1]["state"] == "BUSY" );
        REQUIRE( errorStartEventLog[2]["state"] == "COMPLETED" );
        REQUIRE( errorStartEventLog[3]["state"] == "EXITING" );
        REQUIRE( errorStartEventLog[4]["state"] == "DEPARTED" );

        auto errorEndEvent1Log = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( errorEndEvent1Log[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEvent1Log[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEvent1Log[2]["state"] == "FAILED" );

        auto errorEndEvent2Log = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_2" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( errorEndEvent2Log[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEvent2Log[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEvent2Log[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "FAILING" );
        REQUIRE( processLog[3]["state"] == "FAILED" );
      }
    }
  }
}

SCENARIO( "Non-interrupting escalation throwing error", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "tests/execution/eventsubprocess/Non-interrupting_escalation_throwing_error.bpmn";
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
      engine.run(scenario.get(),0);
      THEN( "The dump of each entry of the token log is correct" ) {
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto escalationStartEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationStartEvent_1" },{"state","ENTERED"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationStartEventLog[0]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog[0]["data"]["instance"] == "Instance_1^EventSubProcess_1#1");
        REQUIRE( escalationStartEventLog[1]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog[1]["data"]["instance"] == "Instance_1^EventSubProcess_1#2");

        auto errorEndEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_2" }});
        REQUIRE( errorEndEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEventLog[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEventLog[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "FAILING" );
        REQUIRE( processLog[3]["state"] == "FAILED" );
      }
    }
  }
}

SCENARIO( "Interrupting escalation throwing error", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "tests/execution/eventsubprocess/Interrupting_escalation_throwing_error.bpmn";
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
      engine.run(scenario.get(),0);
      THEN( "The dump of each entry of the token log is correct" ) {
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto escalationStartEventLog1 = recorder.find(nlohmann::json{{"nodeId","EscalationStartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationStartEventLog1[0]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog1[0]["state"] == "ENTERED" );
        REQUIRE( escalationStartEventLog1[1]["state"] == "BUSY" );
        REQUIRE( escalationStartEventLog1[2]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog1[2]["state"] == "COMPLETED" );
        REQUIRE( escalationStartEventLog1[3]["state"] == "EXITING" );
        REQUIRE( escalationStartEventLog1[4]["state"] == "DEPARTED" );

        auto errorEndEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_2" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( errorEndEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEventLog[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEventLog[2]["state"] == "FAILED" );

        auto escalationEndEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationEndEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationEndEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEndEventLog[1]["state"] == "ENTERED" );
        REQUIRE( escalationEndEventLog[2]["state"] == "WITHDRAWN" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "FAILING" );
        REQUIRE( processLog[3]["state"] == "FAILED" );
      }
    }
  }
}
