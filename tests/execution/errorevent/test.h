SCENARIO( "Error end event", "[execution][error]" ) {
  const std::string modelFile = "execution/errorevent/Simple_error.bpmn";
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
        auto errorEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorEvent_1" }});
        REQUIRE( errorEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( errorEventLog[1]["state"] == "ENTERED" );
        REQUIRE( errorEventLog[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "FAILING" );
        REQUIRE( processLog[3]["state"] == "FAILED" );

/*
        size_t i=0;
        // process
        REQUIRE( recorder.log[i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // error event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEvent_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILING\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
*/
      }
    }
  }
}
