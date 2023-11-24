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
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
      engine.addListener(&recorder);
      engine.run(scenario.get(),0);
      THEN( "The dump of each entry of the recorder log has 17 entries" ) {        
        REQUIRE( recorder.log.size() == 17 );
      }
      THEN( "The dump of each entry of the recorder log is correct" ) {
        // process
        REQUIRE( recorder.log[0].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[1].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // start event
        REQUIRE( recorder.log[2].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[3].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // forking gateway
        REQUIRE( recorder.log[4].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[5].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[6].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"state\":\"TO_BE_COPIED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // end events
        REQUIRE( recorder.log[7]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[8]["state"] == "ARRIVED" );
        REQUIRE( ((std::string)recorder.log[8]["nodeId"]).starts_with("EndEvent_") );
        REQUIRE( recorder.log[9]["state"] == "ENTERED" );
        REQUIRE( recorder.log[10]["state"] == "DONE" );
        REQUIRE( recorder.log[11]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[12]["state"] == "ARRIVED" );
        REQUIRE( ((std::string)recorder.log[12]["nodeId"]).starts_with("EndEvent_") );
        REQUIRE( recorder.log[13]["state"] == "ENTERED" );
        REQUIRE( recorder.log[14]["state"] == "DONE" );
        REQUIRE( recorder.log[8]["nodeId"] != recorder.log[12]["nodeId"] );
        // process
        REQUIRE( recorder.log[15].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[16].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
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
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&entryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&timeHandler);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get(),0);
      THEN( "The dump of each entry of the recorder log has 34 entries" ) {        
        REQUIRE( recorder.log.size() == 34 );
      }
      THEN( "The dump of each entry of the recorder log is correct" ) {
        // process
        REQUIRE( recorder.log[0].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[1].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // start event
        REQUIRE( recorder.log[2].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[3].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // forking gateway
        REQUIRE( recorder.log[4].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[5].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[6].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"state\":\"TO_BE_COPIED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // tasks
        REQUIRE( recorder.log[7]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[8]["state"] == "ARRIVED" );
        REQUIRE( ((std::string)recorder.log[8]["nodeId"]).starts_with("Activity_") );
        REQUIRE( recorder.log[9]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[10]["state"] == "ARRIVED" );
        REQUIRE( ((std::string)recorder.log[10]["nodeId"]).starts_with("Activity_") );
        REQUIRE( recorder.log[11]["state"] == "READY" );
        REQUIRE( recorder.log[12]["state"] == "READY" );
        REQUIRE( recorder.log[13]["state"] == "ENTERED" );
        REQUIRE( recorder.log[14]["state"] == "BUSY" );
        REQUIRE( recorder.log[15]["state"] == "ENTERED" );
        REQUIRE( recorder.log[16]["state"] == "BUSY" );
        REQUIRE( recorder.log[17]["state"] == "COMPLETED" );
        REQUIRE( recorder.log[18]["state"] == "COMPLETED" );
        // exit one task
        REQUIRE( recorder.log[19]["state"] == "EXITING" );
        REQUIRE( recorder.log[20]["state"] == "DEPARTED" );
        // merging gateway
        REQUIRE( recorder.log[21]["state"] == "ARRIVED" );
        REQUIRE( recorder.log[22]["state"] == "HALTED" );
        // exit other task
        REQUIRE( recorder.log[23]["state"] == "EXITING" );
        REQUIRE( recorder.log[24]["state"] == "DEPARTED" );
        // merging gateway
        REQUIRE( recorder.log[25]["state"] == "ARRIVED" );
        REQUIRE( recorder.log[26]["state"] == "HALTED" );
        REQUIRE( recorder.log[27].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_2\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[28].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_2\",\"sequenceFlowId\":\"Flow_0lal75q\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // end event
        REQUIRE( recorder.log[29].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EndEvent_1\",\"sequenceFlowId\":\"Flow_0lal75q\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[30].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EndEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[31].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"EndEvent_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // process
        REQUIRE( recorder.log[32].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"COMPLETED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[33].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"DONE\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
    }
   }
  }
}