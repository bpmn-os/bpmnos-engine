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
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),0);
      THEN( "The dump of each entry of the recorder log has 16 entries" ) {        
        REQUIRE( recorder.log.size() == 16 );
      }
      THEN( "The dump of each entry of the recorder log is correct" ) {
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
      engine.run(scenario.get(),0);
      THEN( "The dump of each entry of the recorder log has 33 entries" ) {        
        REQUIRE( recorder.log.size() == 33 );
      }
      THEN( "The dump of each entry of the recorder log is correct" ) {
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
        // tasks
        REQUIRE( recorder.log[++i]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[++i]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[++i]["state"] == "ARRIVED" );
        REQUIRE( ((std::string)recorder.log[i]["nodeId"]).starts_with("Activity_") );
        REQUIRE( recorder.log[++i]["state"] == "ARRIVED" );
        REQUIRE( ((std::string)recorder.log[i]["nodeId"]).starts_with("Activity_") );
        REQUIRE( recorder.log[++i]["state"] == "READY" );
        REQUIRE( recorder.log[++i]["state"] == "READY" );
        REQUIRE( recorder.log[++i]["state"] == "ENTERED" );
        REQUIRE( recorder.log[++i]["state"] == "BUSY" );
        REQUIRE( recorder.log[++i]["state"] == "COMPLETED" );
        REQUIRE( recorder.log[++i]["state"] == "ENTERED" );
        REQUIRE( recorder.log[++i]["state"] == "BUSY" );
        REQUIRE( recorder.log[++i]["state"] == "COMPLETED" );
        // exit tasks
        REQUIRE( recorder.log[++i]["state"] == "EXITING" );
        REQUIRE( recorder.log[++i]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[++i]["state"] == "ARRIVED" );
        REQUIRE( recorder.log[++i]["state"] == "WAITING" );
        REQUIRE( recorder.log[++i]["state"] == "EXITING" );
        REQUIRE( recorder.log[++i]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[++i]["state"] == "ARRIVED" );
        REQUIRE( recorder.log[++i]["state"] == "WAITING" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_2\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_2\",\"sequenceFlowId\":\"Flow_0lal75q\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // end event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EndEvent_1\",\"sequenceFlowId\":\"Flow_0lal75q\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EndEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EndEvent_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
    }
   }
  }
}

