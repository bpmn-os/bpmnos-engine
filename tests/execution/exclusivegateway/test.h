SCENARIO( "Symmetric exclusive gateways", "[execution][exclusivegateway]" ) {
  const std::string modelFile = "execution/exclusivegateway/Symmetric.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance starting at time 0" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,0\n"
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
      engine.run(scenario.get(),0);
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
        // tasks
        REQUIRE( recorder.log[6]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[7]["state"] == "ARRIVED" );
        REQUIRE( recorder.log[7]["nodeId"] == "Activity_2" );
    }
   }
  }

  GIVEN( "A single instance starting at time 1" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,1\n"
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
      engine.run(scenario.get(),10);
      THEN( "The dump of each entry of the recorder log is correct" ) {
        // process
        REQUIRE( recorder.log[0].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[1].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // start event
        REQUIRE( recorder.log[2].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[3].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // forking gateway
        REQUIRE( recorder.log[4].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[5].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[6].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        // process
        REQUIRE( recorder.log[7].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILING\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[8].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":1.0,\"instance\":\"Instance_1\"}}" );
      }
    }
  }

  GIVEN( "A single instance starting at time 2" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,2\n"
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
      engine.run(scenario.get(),2);
      THEN( "The dump of each entry of the recorder log is correct" ) {
        // process
        REQUIRE( recorder.log[0].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":2.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[1].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":2.0,\"instance\":\"Instance_1\"}}" );
        // start event
        REQUIRE( recorder.log[2].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":2.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[3].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":2.0,\"instance\":\"Instance_1\"}}" );
        // forking gateway
        REQUIRE( recorder.log[4].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"sequenceFlowId\":\"Flow_1ra1q8g\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":2.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[5].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"Gateway_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":2.0,\"instance\":\"Instance_1\"}}" );
        // tasks
        REQUIRE( recorder.log[6]["state"] == "DEPARTED" );
        REQUIRE( recorder.log[7]["state"] == "ARRIVED" );
        REQUIRE( recorder.log[7]["nodeId"] == "Activity_1" );
      }
    }
  }
}
