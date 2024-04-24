SCENARIO( "Parallel fork", "[execution][parallelgateway]" ) {
  const std::string modelFile = "execution/parallelgateway/Fork.bpmn";
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
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),0);
      auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
      THEN( "The dump of each entry of the token log has 16 entries" ) {        
        REQUIRE( tokenLog.size() == 16 );
      }
      THEN( "The dump of each entry of the token log is correct" ) {
        auto gateway1Log = recorder.find(nlohmann::json{{"nodeId","Gateway_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( gateway1Log[0]["state"] == "ARRIVED" );
        REQUIRE( gateway1Log[1]["state"] == "ENTERED" );
        REQUIRE( gateway1Log[2]["state"] == "DEPARTED" );
        REQUIRE( gateway1Log[3]["state"] == "DEPARTED" );
        REQUIRE( gateway1Log[3]["sequenceFlowId"] != gateway1Log[2]["sequenceFlowId"] );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );
/*
        size_t i = 0;
        // process
        REQUIRE( recorder.log[i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // forking gateway
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // end events
        REQUIRE( recorder.log[++i]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[++i]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[++i]["state"] == "ARRIVED" );
        REQUIRE( ((std::string)recorder.log[i]["nodeId"]).starts_with("EndEvent_") );
        REQUIRE( recorder.log[++i]["state"] == "ARRIVED" );
        REQUIRE( ((std::string)recorder.log[i]["nodeId"]).starts_with("EndEvent_") );
        REQUIRE( recorder.log[i-1]["nodeId"] != recorder.log[i]["nodeId"] );
        REQUIRE( recorder.log[++i]["state"] == "ENTERED" );
        REQUIRE( recorder.log[++i]["state"] == "ENTERED" );
        REQUIRE( recorder.log[++i]["state"] == "DONE" );
        REQUIRE( recorder.log[++i]["state"] == "DONE" );
        // process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
*/
      }
   }
  }
}

SCENARIO( "Symmetric parallel gateways", "[execution][parallelgateway]" ) {
  const std::string modelFile = "execution/parallelgateway/Symmetric.bpmn";
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
      auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
      THEN( "The dump of each entry of the token log has 33 entries" ) {        
        REQUIRE( tokenLog.size() == 33 );
      }
      THEN( "The dump of each entry of the token log is correct" ) {
        auto gateway1Log = recorder.find(nlohmann::json{{"nodeId","Gateway_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( gateway1Log[0]["state"] == "ARRIVED" );
        REQUIRE( gateway1Log[1]["state"] == "ENTERED" );
        REQUIRE( gateway1Log[2]["state"] == "DEPARTED" );
        REQUIRE( gateway1Log[3]["state"] == "DEPARTED" );
        REQUIRE( gateway1Log[3]["sequenceFlowId"] != gateway1Log[2]["sequenceFlowId"] );

        auto gateway2Log = recorder.find(nlohmann::json{{"nodeId","Gateway_2" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( gateway2Log[0]["state"] == "ARRIVED" );
        REQUIRE( gateway2Log[1]["state"] == "WAITING" );
        REQUIRE( gateway2Log[2]["state"] == "ARRIVED" );
        REQUIRE( gateway2Log[3]["state"] == "WAITING" );
        REQUIRE( gateway2Log[4]["state"] == "ENTERED" );
        REQUIRE( gateway2Log[5]["state"] == "DEPARTED" );
        
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );
    }
   }
  }
}

