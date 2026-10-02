SCENARIO( "Empty executable process", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Empty_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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
      auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
      THEN( "The token log has exactly 6 entries" ) {
        REQUIRE( tokenLog.size() == 6 );
      }
      THEN( "The first entry of the token log has the correct data" ) {
        REQUIRE( tokenLog.front()["instanceId"] == "Instance_1");
        REQUIRE( tokenLog.front()["processId"] == "Process_1");
        REQUIRE( tokenLog.front()["state"] == "CREATED");
        REQUIRE( tokenLog.front()["data"]["instance"] == "Instance_1");
        REQUIRE( tokenLog.front()["status"]["timestamp"] == 0.0);
      }
      THEN( "The dump of each entry of the token log is correct" ) {
        REQUIRE( tokenLog[0]["state"] == "CREATED" );
        REQUIRE( tokenLog[1]["state"] == "READY" );
        REQUIRE( tokenLog[2]["state"] == "ENTERED" );
        REQUIRE( tokenLog[3]["state"] == "BUSY" );
        REQUIRE( tokenLog[4]["state"] == "COMPLETED" );
        REQUIRE( tokenLog[5]["state"] == "DONE" );
      }
    }
  }
};

SCENARIO( "Trivial executable process", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Trivial_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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
      auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
      THEN( "The token log has exactly 10 entries" ) {
        REQUIRE( tokenLog.size() == 10 );
      }
      THEN( "The dump of each entry of the recorder log is correct" ) {
        REQUIRE( tokenLog[0]["state"] == "CREATED" );
        REQUIRE( tokenLog[1]["state"] == "READY" );
        REQUIRE( tokenLog[2]["state"] == "ENTERED" );
        REQUIRE( tokenLog[3]["state"] == "BUSY" );
        REQUIRE( tokenLog[4]["nodeId"] == "StartEvent_1" );
        REQUIRE( tokenLog[4]["state"] == "ENTERED" );
        REQUIRE( tokenLog[5]["state"] == "BUSY" );
        REQUIRE( tokenLog[6]["state"] == "COMPLETED" );
        REQUIRE( tokenLog[7]["state"] == "DONE" );
        REQUIRE( tokenLog[8]["state"] == "COMPLETED" );
        REQUIRE( tokenLog[9]["state"] == "DONE" );
      }
    }
  }
}


SCENARIO( "Executable process starting after time zero", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Trivial_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance instantiated at time 42" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 42\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    THEN( "The earliest instantiation is at time 42" ) {
      REQUIRE( scenario->getEarliestInstantiationTime() == 42 );
    }

    WHEN( "The engine is run from the earliest instantiation time" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(scenario.get(), scenario->getEarliestInstantiationTime());

      THEN( "The log opens with the clock tick that starts the run" ) {
        REQUIRE( recorder.log[0]["event"] == "clocktick" );
        REQUIRE( recorder.log[0]["time"] == 42 );
      }
      THEN( "The instance is instantiated rather than dropped" ) {
        auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( tokenLog.size() == 10 );
        REQUIRE( tokenLog[0]["instanceId"] == "Instance_1" );
        REQUIRE( tokenLog[0]["state"] == "CREATED" );
        REQUIRE( tokenLog[0]["status"]["timestamp"] == 42.0 );
        REQUIRE( tokenLog[2]["state"] == "ENTERED" );
        REQUIRE( tokenLog[2]["status"]["timestamp"] == 42.0 );
        REQUIRE( tokenLog[9]["state"] == "DONE" );
      }
      THEN( "The run reaches the start time" ) {
        REQUIRE( engine.getSystemState()->getTime() >= 42 );
      }
    }
  }
}

SCENARIO( "Simple executable process", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Simple_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A static instance with timestamp initialization" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 5\n"
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
      engine.run(scenario.get());
      auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
      THEN( "The token log has exactly 20 entries" ) {
        REQUIRE( tokenLog.size() == 20 );
      }
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[2]["status"]["timestamp"] == 5.0);
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[4]["state"] == "COMPLETED" );
        REQUIRE( processLog[5]["state"] == "DONE" );

        auto startLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }});
        REQUIRE( startLog[0]["state"] == "ENTERED" );
        REQUIRE( startLog[1]["state"] == "BUSY" );
        REQUIRE( startLog[2]["state"] == "COMPLETED" );
        REQUIRE( startLog[3]["state"] == "DEPARTED" );

        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( activityLog[0]["state"] == "ARRIVED" );
        REQUIRE( activityLog[1]["state"] == "READY" );
        REQUIRE( activityLog[2]["state"] == "ENTERED" );
        REQUIRE( activityLog[3]["state"] == "BUSY" );
        REQUIRE( activityLog[4]["state"] == "COMPLETED" );
        REQUIRE( activityLog[5]["state"] == "EXITING" );
        REQUIRE( activityLog[6]["state"] == "DEPARTED" );
        
        auto endLog = recorder.find(nlohmann::json{{"nodeId","EndEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( endLog[0]["state"] == "ARRIVED" );
        REQUIRE( endLog[1]["state"] == "ENTERED" );
        REQUIRE( endLog[2]["state"] == "DONE" );
      }
    }
  }
  
  GIVEN( "A dynamic instance with deferred timestamp disclosure" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 10; 5\n"
    ;

    Model::DynamicDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started" ) {
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
      THEN( "The start is deferred correctly" ) {
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[2]["status"]["timestamp"] == 10.0);
      }
    }
  }

  GIVEN( "A stochastic instance with deferred timestamp disclosure" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := triangular(10,10,10); triangular(5,5,5)\n"
    ;

    Model::StochasticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started" ) {
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
      
      THEN( "The start is deferred correctly" ) {
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[2]["status"]["timestamp"] == 10.0);
      }
    }
  }

  GIVEN( "A expected value instance with deferred timestamp disclosure" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := triangular(10,10,10); triangular(5,5,5)\n"
    ;

    Model::ExpectedValueDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started" ) {
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
      
      THEN( "The start is deferred correctly" ) {
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[2]["status"]["timestamp"] == 10.0);
      }
    }
  }
}

SCENARIO( "Constrained executable process", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Constrained_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {
    WHEN( "The engine is started at time 0" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 0\n"
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
      THEN( "The process completes without failure" ) {
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[4]["state"] == "COMPLETED" );
        REQUIRE( processLog[5]["state"] == "DONE" );
      }
    }

    WHEN( "The engine is started at time 2" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 2\n"
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
      THEN( "The process fails after entry" ) {
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[3]["state"] == "FAILED" );
      }
    }

    WHEN( "The engine is started at time 1" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 1\n"
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
      THEN( "The process fails after completion" ) {
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[4]["state"] == "FAILING" );
        REQUIRE( processLog[5]["state"] == "FAILED" );

        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( activityLog[0]["state"] == "ARRIVED" );
        REQUIRE( activityLog[1]["state"] == "READY" );
        REQUIRE( activityLog[2]["state"] == "ENTERED" );
        REQUIRE( activityLog[3]["state"] == "BUSY" );
        REQUIRE( activityLog[4]["state"] == "COMPLETED" );
        REQUIRE( activityLog[5]["state"] == "FAILED" );
      }
    }
  }
}


SCENARIO( "Process with operators", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Process_with_operators.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances of the process" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_1;\n"
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

      THEN( "The operators of the process are applied at its start event" ) {
        auto log = recorder.find(nlohmann::json{{"nodeId","StartEvent_1"},{"state","COMPLETED"}});
        REQUIRE( log.size() == 2 );
        REQUIRE( log[0]["status"]["value"] == 42 );
        REQUIRE( log[1]["status"]["value"] == 42 );
      }

      THEN( "The global written by an operator accumulates over the instances" ) {
        auto log = recorder.find(nlohmann::json{{"nodeId","StartEvent_1"},{"state","COMPLETED"}});
        REQUIRE( log.size() == 2 );
        REQUIRE( log.back()["globals"]["counter"] == 2 );
      }

      THEN( "The timestamp is unchanged by the operators" ) {
        auto log = recorder.find(nlohmann::json{{"nodeId","StartEvent_1"},{"state","COMPLETED"}});
        REQUIRE( log.front()["status"]["timestamp"] == 0.0 );
      }
    }
  }
}

SCENARIO( "Process with operator modifying timestamp", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Process_with_operator_modifying_timestamp.bpmn";

  GIVEN( "A process whose operators modify the timestamp" ) {
    THEN( "The model is refused, the start event applying them being instantaneous" ) {
      REQUIRE_THROWS( Model::Model(modelFile) );
    }
  }
}

SCENARIO( "Executable process created and started in two steps", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Simple_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A static instance with timestamp initialization" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 5\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is initialized at the instantiation time" ) {
      Execution::Engine engine;
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      engine.initialize(scenario.get(), scenario->getEarliestInstantiationTime());
      auto systemState = engine.getSystemState();

      THEN( "The instance is created but not started" ) {
        REQUIRE( systemState->instances.size() == 1 );
        auto instance = systemState->instances.front().get();
        REQUIRE( instance->tokens.size() == 1 );
        auto token = instance->tokens.front().get();
        REQUIRE( token->node == nullptr );
        REQUIRE( token->state == Execution::Token::State::CREATED );
        REQUIRE( token->owned == nullptr );
        REQUIRE( !systemState->archive.contains( (long unsigned int)instance->instance.value() ) );
      }

      WHEN( "The engine advances by the ready event" ) {
        REQUIRE( engine.advance() );

        THEN( "The instance is started" ) {
          REQUIRE( systemState->instances.size() == 1 );
          auto instance = systemState->instances.front().get();
          auto token = instance->tokens.front().get();
          REQUIRE( token->state == Execution::Token::State::BUSY );
          REQUIRE( token->owned != nullptr );
          REQUIRE( systemState->archive.contains( (long unsigned int)instance->instance.value() ) );
        }
      }
    }
  }
}
