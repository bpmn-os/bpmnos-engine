SCENARIO( "Caught error end event", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "execution/eventsubprocess/Caught_error.bpmn";
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
      THEN( "The dump of each entry of the recorder log is correct" ) {

        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto errorStartEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorStartEvent_1" }});
        REQUIRE( errorStartEventLog[0]["state"] == "ENTERED" );
        REQUIRE( errorStartEventLog[1]["state"] == "BUSY" );
        REQUIRE( errorStartEventLog[2]["state"] == "COMPLETED" );
        REQUIRE( errorStartEventLog[3]["state"] == "DONE" );

        auto errorEndEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_1" }});
        REQUIRE( errorEndEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEventLog[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEventLog[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );

      }
    }
  }
}

SCENARIO( "Interrupting escalation", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "execution/eventsubprocess/Interrupting_escalation.bpmn";
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
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto escalationStartEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationStartEvent_1" }});
        REQUIRE( escalationStartEventLog[0]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog[0]["state"] == "ENTERED" );
        REQUIRE( escalationStartEventLog[1]["state"] == "BUSY" );
        REQUIRE( escalationStartEventLog[2]["status"]["timestamp"] == 1.0);
        REQUIRE( escalationStartEventLog[2]["state"] == "COMPLETED" );
        REQUIRE( escalationStartEventLog[3]["state"] == "DONE" );

        auto escalationEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_1" }});
        REQUIRE( escalationEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEventLog[1]["state"] == "ENTERED" );
        REQUIRE( escalationEventLog[2]["state"] == "WITHDRAWN" );

        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }});
        REQUIRE( activityLog[0]["state"] == "ARRIVED" );
        REQUIRE( activityLog[1]["state"] == "READY" );
        REQUIRE( activityLog[2]["state"] == "ENTERED" );
        REQUIRE( activityLog[3]["state"] == "BUSY" );
        REQUIRE( activityLog[4]["state"] == "COMPLETED" );
        REQUIRE( activityLog[5]["state"] == "EXITING" );
        REQUIRE( activityLog[6]["state"] == "DEPARTED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );
      }
    }
  }
}

SCENARIO( "Non-interrupting escalation", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "execution/eventsubprocess/Non-interrupting_escalation.bpmn";
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
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto escalationStartEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationStartEvent_1" },{"state","ENTERED"}});
        REQUIRE( escalationStartEventLog[0]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog[0]["data"]["instance"] == "Instance_1^1");
        REQUIRE( escalationStartEventLog[1]["status"]["timestamp"] == 1.0);
        REQUIRE( escalationStartEventLog[1]["data"]["instance"] == "Instance_1^2");
        
        auto escalationEvent1Log = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_1" }});
        REQUIRE( escalationEvent1Log[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEvent1Log[1]["state"] == "ENTERED" );
        REQUIRE( escalationEvent1Log[2]["state"] == "DEPARTED" );

        auto escalationEvent2Log = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_2" }});
        REQUIRE( escalationEvent2Log[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEvent2Log[1]["state"] == "ENTERED" );
        REQUIRE( escalationEvent2Log[2]["state"] == "DEPARTED" );

        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }});
        REQUIRE( activityLog[0]["state"] == "ARRIVED" );
        REQUIRE( activityLog[1]["state"] == "READY" );
        REQUIRE( activityLog[2]["state"] == "ENTERED" );
        REQUIRE( activityLog[3]["state"] == "BUSY" );
        REQUIRE( activityLog[4]["state"] == "COMPLETED" );
        REQUIRE( activityLog[5]["state"] == "EXITING" );
        REQUIRE( activityLog[6]["state"] == "DEPARTED" );
        
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );

/*
        size_t i=0;
        // process
        REQUIRE( recorder.log[i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // enter escalation start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^1\"}}" );
        // depart from start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_0nwwk78\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // advance escalation start event to busy
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^1\"}}" );
        // task
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Activity_1\",\"sequenceFlowId\":\"Flow_0nwwk78\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Activity_1\",\"state\":\"READY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Activity_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Activity_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Activity_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Activity_1\",\"state\":\"EXITING\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Activity_1\",\"sequenceFlowId\":\"Flow_18gfgxb\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // first escalation throw
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEvent_1\",\"sequenceFlowId\":\"Flow_18gfgxb\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // trigger event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1^1\"}}" );
        // respawn event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^2\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1^2\"}}" );
        // depart escalation throw
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEvent_1\",\"sequenceFlowId\":\"Flow_0ly84o6\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // advance 2nd event subprocess instance to busy
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^2\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1^2\"}}" );
        // advance 1st event subprocess instance to done
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1^1\"}}" );
        // second escalation throw
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEvent_2\",\"sequenceFlowId\":\"Flow_0ly84o6\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEvent_2\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // trigger 2nd event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^2\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1^2\"}}" );
        // respawn 3rd event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^3\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1^3\"}}" );
        // depart 2nd escalation throw
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEvent_2\",\"sequenceFlowId\":\"Flow_17wiwii\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // advance 3rd event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^3\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1^3\"}}" );
        // advance 2nd event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^2\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1^2\"}}" );
        // arrive at end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EndEvent_1\",\"sequenceFlowId\":\"Flow_17wiwii\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EndEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EndEvent_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // withdraw 3rd event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^3\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"WITHDRAWN\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1^3\"}}" );
        // process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
*/
      }
    }
  }
}

SCENARIO( "Caught and rethrown error", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "execution/eventsubprocess/Caught_and_rethrown_error.bpmn";
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto errorStartEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorStartEvent_1" }});
        REQUIRE( errorStartEventLog[0]["state"] == "ENTERED" );
        REQUIRE( errorStartEventLog[1]["state"] == "BUSY" );
        REQUIRE( errorStartEventLog[2]["state"] == "COMPLETED" );
        REQUIRE( errorStartEventLog[3]["state"] == "DEPARTED" );

        auto errorEndEvent1Log = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_1" }});
        REQUIRE( errorEndEvent1Log[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEvent1Log[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEvent1Log[2]["state"] == "FAILED" );

        auto errorEndEvent2Log = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_2" }});
        REQUIRE( errorEndEvent2Log[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEvent2Log[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEvent2Log[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "FAILING" );
        REQUIRE( processLog[3]["state"] == "FAILED" );
/*
        size_t i= 0;
        // enter process and event subprocess
        REQUIRE( recorder.log[i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // advance to busy process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // enter start events
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorStartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // depart start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // advance to busy event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorStartEvent_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // error end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEndEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEndEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEndEvent_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // error start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorStartEvent_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // error start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorStartEvent_1\",\"sequenceFlowId\":\"Flow_1wnmtqd\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // error end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEndEvent_2\",\"sequenceFlowId\":\"Flow_1wnmtqd\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEndEvent_2\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEndEvent_2\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILING\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
*/
      }
    }
  }
}

SCENARIO( "Non-interrupting escalation throwing error", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "execution/eventsubprocess/Non-interrupting_escalation_throwing_error.bpmn";
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto escalationStartEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationStartEvent_1" },{"state","ENTERED"}});
        REQUIRE( escalationStartEventLog[0]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog[0]["data"]["instance"] == "Instance_1^1");
        REQUIRE( escalationStartEventLog[1]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog[1]["data"]["instance"] == "Instance_1^2");

        auto errorEndEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_2" }});
        REQUIRE( errorEndEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEventLog[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEventLog[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "FAILING" );
        REQUIRE( processLog[3]["state"] == "FAILED" );
  /*    
        size_t i= 0;
        // enter process and event subprocess
        REQUIRE( recorder.log[i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // advance to busy process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // enter start events
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^1\"}}" );
        // depart start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // advance to busy event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^1\"}}" );
        // escalation end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEndEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEndEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // escalation start events
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^2\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^2\"}}" );
        // escalation end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEndEvent_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // escalation start events
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^2\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^2\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"EscalationStartEvent_1\",\"sequenceFlowId\":\"Flow_1wnmtqd\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^1\"}}" );
        // error end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"ErrorEndEvent_2\",\"sequenceFlowId\":\"Flow_1wnmtqd\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"ErrorEndEvent_2\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1^1\",\"nodeId\":\"ErrorEndEvent_2\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1^1\"}}" );
        // process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILING\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
*/
      }
    }
  }
}

SCENARIO( "Interrupting escalation throwing error", "[execution][eventsubprocess]" ) {
  const std::string modelFile = "execution/eventsubprocess/Interrupting_escalation_throwing_error.bpmn";
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
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }});
        REQUIRE( startEventLog[0]["state"] == "ENTERED" );
        REQUIRE( startEventLog[1]["state"] == "DEPARTED" );
        
        auto escalationStartEventLog1 = recorder.find(nlohmann::json{{"nodeId","EscalationStartEvent_1" }});
        REQUIRE( escalationStartEventLog1[0]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog1[0]["state"] == "ENTERED" );
        REQUIRE( escalationStartEventLog1[1]["state"] == "BUSY" );
        REQUIRE( escalationStartEventLog1[2]["status"]["timestamp"] == 0.0);
        REQUIRE( escalationStartEventLog1[2]["state"] == "COMPLETED" );
        REQUIRE( escalationStartEventLog1[3]["state"] == "DEPARTED" );

        auto errorEndEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_2" }});
        REQUIRE( errorEndEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( errorEndEventLog[1]["state"] == "ENTERED" );
        REQUIRE( errorEndEventLog[2]["state"] == "FAILED" );

        auto escalationEndEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationEndEvent_1" }});
        REQUIRE( escalationEndEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEndEventLog[1]["state"] == "ENTERED" );
        REQUIRE( escalationEndEventLog[2]["state"] == "WITHDRAWN" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "FAILING" );
        REQUIRE( processLog[3]["state"] == "FAILED" );
/*
        size_t i= 0;
        // enter process and event subprocess
        REQUIRE( recorder.log[i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // advance to busy process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // enter start events
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // depart start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // advance to busy event subprocess
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // escalation end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEndEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEndEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // escalation start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationStartEvent_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // escalation end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationEndEvent_1\",\"state\":\"WITHDRAWN\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // escalation start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EscalationStartEvent_1\",\"sequenceFlowId\":\"Flow_1wnmtqd\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // error end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEndEvent_2\",\"sequenceFlowId\":\"Flow_1wnmtqd\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEndEvent_2\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEndEvent_2\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILING\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
*/
      }
    }
  }
}
