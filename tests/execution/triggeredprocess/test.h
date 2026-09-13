SCENARIO( "Signal triggered process with operators", "[execution][triggeredprocess]" ) {
  const std::string modelFile = "tests/execution/triggeredprocess/Signal_triggered_process_with_operators.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One emitter" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Emitter; amount := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The emitter throws a signal nothing else awaits" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());

      THEN( "An instance of the triggered process is created nonetheless" ) {
        auto log = recorder.find(nlohmann::json{{"processId","Triggered"},{"nodeId","SignalStartEvent"},{"state","COMPLETED"}});
        REQUIRE( log.size() == 1 );
      }
    }
  }

  GIVEN( "An emitter of a signal no start event declares" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; OtherEmitter; amount := 3\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());

      THEN( "The emitter completes" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId","OtherEmitter"},{"nodeId","ThrowEvent_2"},{"state","DONE"}}).size() == 1 );
      }

      THEN( "No instance is created by the signal" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId","Triggered"}}).size() == 0 );
      }

      THEN( "The operators of the triggered process are not applied" ) {
        auto log = recorder.find(nlohmann::json{{"processId","OtherEmitter"},{"nodeId","ThrowEvent_2"},{"state","DONE"}});
        REQUIRE( log.size() == 1 );
        REQUIRE( log.back()["globals"]["instantiations"] == 0 );
        REQUIRE( log.back()["globals"]["total"] == 0 );
      }
    }
  }

  GIVEN( "Two emitters" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Emitter; amount := 7\n"
      "Instance_2; Emitter; amount := 5\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());

      auto log = recorder.find(nlohmann::json{{"processId","Triggered"},{"nodeId","SignalStartEvent"},{"state","COMPLETED"}});

      THEN( "One instance of the triggered process is created per signal thrown" ) {
        REQUIRE( log.size() == 2 );
      }

      THEN( "Each instance receives a generated identifier of its own" ) {
        REQUIRE( log.size() == 2 );
        REQUIRE( log[0]["data"]["instance"] == "Triggered#1" );
        REQUIRE( log[1]["data"]["instance"] == "Triggered#2" );
      }

      THEN( "The content of the signal is part of the status the instance is created with" ) {
        REQUIRE( log.size() == 2 );
        REQUIRE( ( log[0]["status"]["received"] == 7 || log[0]["status"]["received"] == 5 ) );
        REQUIRE( ( log[1]["status"]["received"] == 7 || log[1]["status"]["received"] == 5 ) );
        REQUIRE( log[0]["status"]["received"] != log[1]["status"]["received"] );
      }

      THEN( "The operators of the triggered process are applied on instantiation" ) {
        REQUIRE( log.size() == 2 );
        REQUIRE( log.back()["globals"]["instantiations"] == 2 );
        REQUIRE( log.back()["globals"]["total"] == 12 );
      }
    }
  }
}
