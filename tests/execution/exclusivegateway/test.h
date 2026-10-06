SCENARIO( "Symmetric exclusive gateways", "[execution][exclusivegateway]" ) {
  const std::string modelFile = "tests/execution/exclusivegateway/Symmetric.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance starting at time 0" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(0);
      engine.run(std::move(scenario));
      THEN( "The dump of each entry of the token log is correct" ) {
        auto gatewayLog = recorder.find(nlohmann::json{{"nodeId","Gateway_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( gatewayLog[0]["state"] == "ARRIVED" );
        REQUIRE( gatewayLog[1]["state"] == "ENTERED" );
        REQUIRE( gatewayLog[2]["state"] == "DEPARTED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[4]["state"] == "COMPLETED" );
        REQUIRE( processLog[5]["state"] == "DONE" );
        REQUIRE( recorder.log[14]["nodeId"] == "Activity_2" );
    }
   }
  }

  GIVEN( "A single instance starting at time 1" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(10);
      engine.run(std::move(scenario));
      THEN( "The dump of each entry of the token log is correct" ) {
        auto gatewayLog = recorder.find(nlohmann::json{{"nodeId","Gateway_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( gatewayLog[0]["state"] == "ARRIVED" );
        REQUIRE( gatewayLog[1]["state"] == "ENTERED" );
        REQUIRE( gatewayLog[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[4]["state"] == "FAILING" );
        REQUIRE( processLog[5]["state"] == "FAILED" );
      }
    }
  }

  GIVEN( "A single instance starting at time 2" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 2\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(2);
      engine.run(std::move(scenario));
      THEN( "The dump of each entry of the token log is correct" ) {
        auto gatewayLog = recorder.find(nlohmann::json{{"nodeId","Gateway_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( gatewayLog[0]["state"] == "ARRIVED" );
        REQUIRE( gatewayLog[1]["state"] == "ENTERED" );
        REQUIRE( gatewayLog[2]["state"] == "DEPARTED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[4]["state"] == "COMPLETED" );
        REQUIRE( processLog[5]["state"] == "DONE" );

        auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( tokenLog[11]["nodeId"] == "Activity_1" );
      }
    }
  }
}
