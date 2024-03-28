SCENARIO( "Task with expression operator", "[data]" ) {
  const std::string modelFile = "execution/data/Data.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,DataAttribute_1,8\n"
      "Process_1, Instance_1,DataAttribute_2,15\n"
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
//      Execution::Recorder recorder;
      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The status updates are correct" ) {
/*
        auto subProcessLog = recorder.find(nlohmann::json{{"nodeId", "SubProcess_1"}});
        REQUIRE( subProcessLog[0]["state"] == "ARRIVED" );
        REQUIRE( subProcessLog[1]["state"] == "READY" );
        REQUIRE( subProcessLog[1]["status"].size() == subProcessLog[0]["status"].size()+1 );
        REQUIRE( subProcessLog[subProcessLog.size()-2]["state"] == "EXITING" );
        REQUIRE( subProcessLog[subProcessLog.size()-1]["state"] == "DEPARTED" );
        REQUIRE( subProcessLog[1]["status"].size() == subProcessLog[subProcessLog.size()-2]["status"].size() );
        REQUIRE( subProcessLog[subProcessLog.size()-1]["status"].size() == subProcessLog[subProcessLog.size()-2]["status"].size()-1 );

        auto taskLog = recorder.find(nlohmann::json{{"nodeId", "Task_1"}});
        REQUIRE( taskLog[0]["state"] == "ARRIVED" );
        REQUIRE( taskLog[1]["state"] == "READY" );
        REQUIRE( taskLog[0]["status"].size() == subProcessLog[1]["status"].size() );
        REQUIRE( taskLog[1]["status"].size() == taskLog[0]["status"].size()+1 );
        REQUIRE( taskLog[taskLog.size()-2]["state"] == "EXITING" );
        REQUIRE( taskLog[taskLog.size()-1]["state"] == "DEPARTED" );
        REQUIRE( taskLog[1]["status"].size() == taskLog[taskLog.size()-2]["status"].size() );
        REQUIRE( taskLog[taskLog.size()-1]["status"].size() == taskLog[taskLog.size()-2]["status"].size()-1 );
        REQUIRE( taskLog[taskLog.size()-1]["status"].size() == subProcessLog[subProcessLog.size()-2]["status"].size() );
*/
      }
    }
  }
}
