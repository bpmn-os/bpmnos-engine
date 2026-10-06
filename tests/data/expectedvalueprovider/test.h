SCENARIO( "Expected value data provider", "[data][expected]" ) {
  auto model = std::make_shared<const Model::Model>("tests/execution/process/Empty_executable_process.bpmn");

  GIVEN( "A single instance whose timestamp is uniformly distributed between 2 and 4, with all columns" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := uniform(2,4); 1;;\n"
    ;
    auto dataProvider = std::make_shared<Execution::ExpectedValueDataProvider>(model, csv);

    WHEN( "The engine runs on a scenario of the data provider" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The process becomes ready at the expected timestamp, the disclosure being ignored" ) {
        auto readyLog = recorder.find(nlohmann::json{{"processId","Process_1"},{"state","READY"}});
        REQUIRE( readyLog.size() == 1 );
        REQUIRE( readyLog.front()["status"]["timestamp"] == 3.0 );
      }
      THEN( "The run ends with a termination event" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
      }
    }
  }

  GIVEN( "The same instance in a table with three columns" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := uniform(2,4)\n"
    ;
    auto dataProvider = std::make_shared<Execution::ExpectedValueDataProvider>(model, csv);

    WHEN( "The engine runs on a scenario of the data provider" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The process becomes ready at the expected timestamp" ) {
        auto readyLog = recorder.find(nlohmann::json{{"processId","Process_1"},{"state","READY"}});
        REQUIRE( readyLog.size() == 1 );
        REQUIRE( readyLog.front()["status"]["timestamp"] == 3.0 );
      }
    }
  }
}
