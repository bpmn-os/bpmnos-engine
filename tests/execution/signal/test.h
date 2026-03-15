SCENARIO( "Two processes with signal", "[execution][signal]" ) {
  const std::string modelFile = "tests/execution/signal/Signal.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started with one emitter before one recipient" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 0\n"
        "Instance_2; Process_2; timestamp := 1\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),2);
      THEN( "The signal is emitted before it can be received" ) {
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( recipientLog.size() == 0 ); 
      }
    }

    WHEN( "The engine is started with one emitter after one recipient" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 1\n"
        "Instance_2; Process_2; timestamp := 0\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The signal is received" ) {
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( recipientLog.size() == 1 ); 
        REQUIRE( recipientLog.front()["status"]["emitter"] == "Instance_1" ); 
      }
    }

    WHEN( "The engine is started with one emitter after two recipients" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 1\n"
        "Instance_2; Process_2; timestamp := 0\n"
        "Instance_3; Process_2; timestamp := 0\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The signal is received by both recipients" ) {
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( recipientLog.size() == 2 ); 
        REQUIRE( recipientLog.front()["status"]["emitter"] == "Instance_1" ); 
        REQUIRE( recipientLog.back()["status"]["emitter"] == "Instance_1" ); 
      }
    }

    WHEN( "The engine is started with two emitters after two recipients" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_0; Process_1; timestamp := 1\n"
        "Instance_1; Process_1; timestamp := 1\n"
        "Instance_2; Process_2; timestamp := 0\n"
        "Instance_3; Process_2; timestamp := 0\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The one of the signals is received by both recipients" ) {
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( recipientLog.size() == 2 ); 
        REQUIRE( recipientLog.front()["status"]["emitter"] == recipientLog.back()["status"]["emitter"] );
      }
    }

  }
}
