SCENARIO( "Event-based gateway with two timer events", "[execution][eventbasedgateway]" ) {
  const std::string modelFile = "tests/execution/eventbasedgateway/Two_timer_events.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "Timer 1 triggers before Timer 2" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; trigger1 := 1\n"
        "Instance_1; Process_1; trigger2 := 2\n"
      ;

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token at timer 2 is withdrawn" ) {
        auto completionLog =recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "TimerEvent_1" );
        REQUIRE( completionLog[1]["nodeId"] == "Gateway_1" );
        REQUIRE( completionLog[2]["nodeId"] == nullptr );

        auto withdrawnLog =recorder.find(nlohmann::json{{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog[0]["nodeId"] == "TimerEvent_2" );
      }
    }

    WHEN( "Timer 2 triggers before Timer 1" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; trigger1 := 2\n"
        "Instance_1; Process_1; trigger2 := 1\n"
      ;

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token at timer 1 is withdrawn" ) {
        auto completionLog =recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "TimerEvent_2" );
        REQUIRE( completionLog[1]["nodeId"] == "Gateway_1" );
        REQUIRE( completionLog[2]["nodeId"] == nullptr );

        auto withdrawnLog =recorder.find(nlohmann::json{{"state", "WITHDRAWN"}});
        REQUIRE( withdrawnLog[0]["nodeId"] == "TimerEvent_1" );
      }
    }
  }
}
