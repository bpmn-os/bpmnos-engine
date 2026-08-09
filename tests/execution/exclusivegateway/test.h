SCENARIO( "Symmetric exclusive gateways", "[execution][exclusivegateway]" ) {
  const std::string modelFile = "tests/execution/exclusivegateway/Symmetric.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance starting at time 0" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(), 0, 0);
      THEN( "The dump of each entry of the token log is correct" ) {
        auto gatewayLog = recorder.find(nlohmann::json{{"nodeId","Gateway_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( gatewayLog[0]["state"] == "ARRIVED" );
        REQUIRE( gatewayLog[1]["state"] == "ENTERED" );
        REQUIRE( gatewayLog[2]["state"] == "DEPARTED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );
        REQUIRE( recorder.log[8]["nodeId"] == "Activity_2" );
    }
   }
  }

  GIVEN( "A single instance starting at time 1" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(), 0, 10);
      THEN( "The dump of each entry of the token log is correct" ) {
        auto gatewayLog = recorder.find(nlohmann::json{{"nodeId","Gateway_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( gatewayLog[0]["state"] == "ARRIVED" );
        REQUIRE( gatewayLog[1]["state"] == "ENTERED" );
        REQUIRE( gatewayLog[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "FAILING" );
        REQUIRE( processLog[3]["state"] == "FAILED" );
      }
    }
  }

  GIVEN( "A single instance starting at time 2" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 2\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(), 0, 2);
      THEN( "The dump of each entry of the token log is correct" ) {
        auto gatewayLog = recorder.find(nlohmann::json{{"nodeId","Gateway_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( gatewayLog[0]["state"] == "ARRIVED" );
        REQUIRE( gatewayLog[1]["state"] == "ENTERED" );
        REQUIRE( gatewayLog[2]["state"] == "DEPARTED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );

        auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( tokenLog[7]["nodeId"] == "Activity_1" );
      }
    }
  }
}
