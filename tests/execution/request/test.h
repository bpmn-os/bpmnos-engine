SCENARIO( "A simple request", "[execution][request]" ) {
  const std::string modelFile = "tests/execution/request/Simple_request.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started with one request and one recipient" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Timestamp,0\n"
        "Process_2, Instance_2,Timestamp,1\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      messageHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),2);
      THEN( "The request and the recipient process are completed" ) {
        auto requestLog =recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state", "COMPLETED"}});
        REQUIRE( requestLog.size() == 1 ); 
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","EndEvent_2"},{"state", "DONE"}});
        REQUIRE( recipientLog.size() == 1 );         
      }
    }
  }
}

SCENARIO( "A revoked request", "[execution][request]" ) {
  const std::string modelFile = "tests/execution/request/Revoked_request.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started with one request and one recipient" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Timestamp,0\n"
        "Process_2, Instance_2,Timestamp,1\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      messageHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),2);
      THEN( "The request is revoked and the recipient process receives the revoke message" ) {
        auto requestLog =recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state", "COMPLETED"}});
        REQUIRE( requestLog.size() == 0 ); 
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","EndEvent_2"},{"state", "DONE"}});
        REQUIRE( recipientLog.size() == 1 );         
      }
    }
  }
}
