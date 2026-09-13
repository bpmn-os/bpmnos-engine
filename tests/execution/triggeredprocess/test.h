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

SCENARIO( "Message triggered process", "[execution][triggeredprocess]" ) {
  const std::string modelFile = "tests/execution/triggeredprocess/Message_triggered_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One emitter throwing the message at an event" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; ThrowEventEmitter; amount := 7\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started without any message handler" ) {
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

      auto log = recorder.find(nlohmann::json{{"processId","Triggered"},{"nodeId","MessageStartEvent"},{"state","COMPLETED"}});

      THEN( "An instance of the triggered process is created with a generated identifier" ) {
        REQUIRE( log.size() == 1 );
        REQUIRE( log[0]["data"]["instance"] == "Triggered#1" );
      }

      THEN( "The content of the message is part of the status the instance is created with" ) {
        REQUIRE( log.size() == 1 );
        REQUIRE( log[0]["status"]["received"] == 7 );
      }

      THEN( "The operators of the triggered process are applied on instantiation" ) {
        REQUIRE( log.size() == 1 );
        REQUIRE( log[0]["globals"]["instantiations"] == 1 );
        REQUIRE( log[0]["globals"]["total"] == 7 );
      }

      THEN( "The message is delivered although no message handler is connected" ) {
        REQUIRE( recorder.find(nlohmann::json{{"origin","ThrowEvent_1"},{"state","CREATED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"origin","ThrowEvent_1"},{"state","DELIVERED"}}).size() == 1 );
      }

      THEN( "The emitter completes" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId","ThrowEventEmitter"},{"nodeId","ThrowEvent_1"},{"state","DONE"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Two emitters throwing the message at an event" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; ThrowEventEmitter; amount := 7\n"
      "Instance_2; ThrowEventEmitter; amount := 5\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started without any message handler" ) {
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

      auto log = recorder.find(nlohmann::json{{"processId","Triggered"},{"nodeId","MessageStartEvent"},{"state","COMPLETED"}});

      THEN( "One instance of the triggered process is created per message thrown" ) {
        REQUIRE( log.size() == 2 );
      }

      THEN( "Each instance receives a generated identifier of its own" ) {
        REQUIRE( log.size() == 2 );
        REQUIRE( log[0]["data"]["instance"] == "Triggered#1" );
        REQUIRE( log[1]["data"]["instance"] == "Triggered#2" );
      }

      THEN( "The content of each message is part of the status the respective instance is created with" ) {
        REQUIRE( log.size() == 2 );
        REQUIRE( ( log[0]["status"]["received"] == 7 || log[0]["status"]["received"] == 5 ) );
        REQUIRE( ( log[1]["status"]["received"] == 7 || log[1]["status"]["received"] == 5 ) );
        REQUIRE( log[0]["status"]["received"] != log[1]["status"]["received"] );
      }

      THEN( "The operators of the triggered process are applied on each instantiation" ) {
        REQUIRE( log.size() == 2 );
        REQUIRE( log.back()["globals"]["instantiations"] == 2 );
        REQUIRE( log.back()["globals"]["total"] == 12 );
      }
    }
  }

  GIVEN( "An emitter throwing the message at a send task" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; SendTaskEmitter; amount := 3\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started without any message handler" ) {
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

      THEN( "An instance of the triggered process is created" ) {
        auto log = recorder.find(nlohmann::json{{"processId","Triggered"},{"nodeId","MessageStartEvent"},{"state","COMPLETED"}});
        REQUIRE( log.size() == 1 );
        REQUIRE( log[0]["data"]["instance"] == "Triggered#1" );
        REQUIRE( log[0]["status"]["received"] == 3 );
      }

      THEN( "The send task is completed by the instantiation" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId","SendTaskEmitter"},{"nodeId","SendTask_1"},{"state","COMPLETED"}}).size() == 1 );
      }

      THEN( "The emitter proceeds beyond the send task" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId","SendTaskEmitter"},{"nodeId","EndEvent_2"},{"state","DONE"}}).size() == 1 );
      }
    }
  }

  GIVEN( "An emitter throwing the message at a multi-instance send task" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; MultiSendEmitter; timestamp := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started without any message handler" ) {
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

      auto log = recorder.find(nlohmann::json{{"processId","Triggered"},{"nodeId","MessageStartEvent"},{"state","COMPLETED"}});

      THEN( "One instance of the triggered process is created per instance of the send task" ) {
        REQUIRE( log.size() == 2 );
        REQUIRE( log[0]["data"]["instance"] == "Triggered#1" );
        REQUIRE( log[1]["data"]["instance"] == "Triggered#2" );
      }

      THEN( "The content each instance of the send task provides reaches the instance it creates" ) {
        REQUIRE( log.size() == 2 );
        REQUIRE( ( log[0]["status"]["received"] == 1 || log[0]["status"]["received"] == 2 ) );
        REQUIRE( ( log[1]["status"]["received"] == 1 || log[1]["status"]["received"] == 2 ) );
        REQUIRE( log[0]["status"]["received"] != log[1]["status"]["received"] );
        REQUIRE( log.back()["globals"]["total"] == 3 );
      }

      THEN( "The emitter proceeds beyond the multi-instance send task" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId","MultiSendEmitter"},{"nodeId","EndEvent_3"},{"state","DONE"}}).size() == 1 );
      }
    }
  }

  GIVEN( "An emitter of a message no start event declares" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; OtherEmitter; amount := 3\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started without any message handler" ) {
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

      THEN( "The emitter completes" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId","OtherEmitter"},{"nodeId","ThrowEvent_2"},{"state","DONE"}}).size() == 1 );
      }

      THEN( "No instance is created by the message" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId","Triggered"}}).size() == 0 );
      }

      THEN( "The message is not delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"origin","ThrowEvent_2"},{"state","CREATED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"origin","ThrowEvent_2"},{"state","DELIVERED"}}).size() == 0 );
      }
    }
  }
}
