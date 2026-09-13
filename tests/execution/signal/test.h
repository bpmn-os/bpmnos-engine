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
      THEN( "The signal is emitted before it can be received" ) {
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( recipientLog.size() == 0 );
      }
      AND_THEN( "The signal is recorded although nobody receives it" ) {
        auto signalLog = recorder.find(nlohmann::json{{"name","Signal"}});
        REQUIRE( signalLog.size() == 1 );
        REQUIRE( !signalLog.front()["content"]["Emitter"].is_null() );
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
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
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
      AND_THEN( "The signal is recorded once, with the content it carries" ) {
        auto signalLog = recorder.find(nlohmann::json{{"name","Signal"}});
        REQUIRE( signalLog.size() == 1 );
        REQUIRE( !signalLog.front()["content"]["Emitter"].is_null() );
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
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
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
      AND_THEN( "One signal is recorded, however many receive it" ) {
        REQUIRE( recorder.find(nlohmann::json{{"name","Signal"}}).size() == 1 );
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
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
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
      AND_THEN( "Each emitter records a signal of its own, the second reaching nobody" ) {
        auto signalLog = recorder.find(nlohmann::json{{"name","Signal"}});
        REQUIRE( signalLog.size() == 2 );
        REQUIRE( signalLog.front()["content"]["Emitter"] != signalLog.back()["content"]["Emitter"] );
      }
    }

  }
}

SCENARIO( "Signal raised by the environment", "[execution][signal]" ) {
  const std::string modelFile = "tests/execution/signal/Signal.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A recipient and no emitter" ) {
    // only Process_2 is instantiated, so nothing in the model ever throws the signal
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_2; Process_2; timestamp := 0\n"
    ;
    auto signalName = BPMNOS::to_number(std::string("Signal"),STRING);

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0, 0);
    REQUIRE( engine.getSystemState()->tokensAwaitingSignal.count(signalName) == 1 );

    WHEN( "The environment raises the signal" ) {
      BPMNOS::VariedValueMap content;
      content.emplace( "Emitter", std::string("World") );
      engine.injectSignal( Execution::Signal(signalName,content) );

      THEN( "The signal is recorded like one thrown within the model" ) {
        auto signalLog = recorder.find(nlohmann::json{{"name","Signal"}});
        REQUIRE( signalLog.size() == 1 );
        REQUIRE( signalLog.front()["content"]["Emitter"] == "World" );
      }

      AND_THEN( "The waiting token receives it" ) {
        auto recipientLog = recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state","COMPLETED"}});
        REQUIRE( recipientLog.size() == 1 );
        REQUIRE( recipientLog.front()["status"]["emitter"] == "World" );
      }
    }
  }
}
