#include "prelude.h"

SCENARIO( "Empty executable process", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Empty_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
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

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
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

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    THEN( "The earliest instantiation is at time 42" ) {
      REQUIRE( dataProvider->getEarliestInstantiationTime(*scenario) == 42 );
    }

    WHEN( "The engine is run from the earliest instantiation time" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      auto startTime = dataProvider->getEarliestInstantiationTime(*scenario);
      engine.run(std::move(scenario), startTime);

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
      engine.run(std::move(scenario));
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

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::DynamicDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
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

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      
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

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::ExpectedValueDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      
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

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

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

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is initialized at the instantiation time and advances by the instantiation event" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      auto startTime = dataProvider->getEarliestInstantiationTime(*scenario);
      engine.initialize(std::move(scenario), startTime);
      auto systemState = engine.getSystemState();
      REQUIRE( systemState->stateMachine->tokens.empty() );
      REQUIRE( engine.advance() );

      THEN( "The instantiation event is dispatched before the token at the process is created" ) {
        REQUIRE( recorder.log[0]["event"] == "clocktick" );
        REQUIRE( recorder.log[1]["event"] == "instantiation" );
        REQUIRE( recorder.log[1]["processId"] == "Process_1" );
        REQUIRE( recorder.log[1]["instanceId"] == "Instance_1" );
        REQUIRE( recorder.log[2]["state"] == "CREATED" );
      }

      THEN( "The instance is created but not started" ) {
        REQUIRE( systemState->stateMachine->tokens.size() == 1 );
        auto token = systemState->stateMachine->tokens.front().get();
        REQUIRE( token->node->represents<BPMN::Process>() );
        REQUIRE( token->state == Execution::Token::State::CREATED );
        // the state machine of the instance holds its data from the instantiation on, but no token
        REQUIRE( token->owned != nullptr );
        REQUIRE( token->owned->tokens.empty() );
        REQUIRE( !systemState->archive.contains( (long unsigned int)token->owned->instance.value() ) );
      }

      WHEN( "The engine advances by the ready event" ) {
        REQUIRE( engine.advance() );

        THEN( "The instance is started" ) {
          REQUIRE( systemState->stateMachine->tokens.size() == 1 );
          auto token = systemState->stateMachine->tokens.front().get();
          REQUIRE( token->state == Execution::Token::State::BUSY );
          REQUIRE( token->owned != nullptr );
          REQUIRE( systemState->archive.contains( (long unsigned int)token->owned->instance.value() ) );
        }
      }
    }
  }
}

SCENARIO( "Executable process created when it becomes known", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Simple_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A dynamic instance disclosed at time 5 and instantiated at time 10" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 10; 5\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::DynamicDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is run until time 5" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      dataProvider->setEndTime(5);
      engine.run(std::move(scenario));
      auto systemState = engine.getSystemState();

      THEN( "The instance is created but not started" ) {
        REQUIRE( systemState->stateMachine->tokens.size() == 1 );
        auto token = systemState->stateMachine->tokens.front().get();
        REQUIRE( token->state == Execution::Token::State::CREATED );
        REQUIRE( token->owned->tokens.empty() );
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.size() == 1 );
        REQUIRE( processLog[0]["state"] == "CREATED" );
      }

      WHEN( "The engine is resumed until time 10" ) {
        dataProvider->setEndTime(10);
        engine.resume();

        THEN( "The instance is started at time 10" ) {
          auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
          REQUIRE( processLog.size() >= 3 );
          REQUIRE( processLog[0]["state"] == "CREATED" );
          REQUIRE( processLog[1]["state"] == "READY" );
          REQUIRE( processLog[1]["status"]["timestamp"] == 10.0 );
          REQUIRE( processLog[2]["state"] == "ENTERED" );
          REQUIRE( processLog[2]["status"]["timestamp"] == 10.0 );
        }
      }
    }
  }
}

SCENARIO( "Executable process known before its start", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Process_with_weighted_data.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A static instance with weighted data instantiated at time 10" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 10\n"
      "Instance_1; Process_1; cost := 7\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is run until time 5" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      dataProvider->setEndTime(5);
      engine.run(std::move(scenario));
      auto systemState = engine.getSystemState();

      THEN( "The instance is known but raises no decision request" ) {
        REQUIRE( systemState->stateMachine->tokens.size() == 1 );
        REQUIRE( systemState->stateMachine->tokens.front()->state == Execution::Token::State::CREATED );
        REQUIRE( recorder.find(nlohmann::json{{"decision",nullptr}}).empty() );
        REQUIRE( systemState->pendingEntryDecisions.empty() );
        REQUIRE( systemState->pendingExitDecisions.empty() );
      }

      THEN( "The data of the instance is not yet accounted in the objective" ) {
        REQUIRE( systemState->getObjective() == 0 );
      }

      WHEN( "The engine is resumed until time 10" ) {
        dataProvider->setEndTime(10);
        engine.resume();

        THEN( "The data of the instance is accounted in the objective from its start" ) {
          REQUIRE( systemState->getObjective() == -7 );
        }
      }
    }
  }
}

SCENARIO( "Engine refusing a scenario of another model", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Empty_executable_process.bpmn";

  GIVEN( "Two data providers each built on a model parsed from the model file" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto otherModel = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto otherDataProvider = std::make_shared<Execution::StaticDataProvider>(otherModel, csv);

    WHEN( "The engine is constructed with the model of the first data provider" ) {
      Execution::Engine engine(model);

      THEN( "The engine executes the model of the first data provider" ) {
        REQUIRE( engine.getModel() == model.get() );
      }
      THEN( "The engine runs a scenario of the first data provider" ) {
        REQUIRE_NOTHROW( engine.run(dataProvider->createScenario()) );
      }
      THEN( "The engine refuses to run a scenario of the second data provider" ) {
        REQUIRE_THROWS_AS( engine.run(otherDataProvider->createScenario()), std::invalid_argument );
      }
      THEN( "The engine refuses to install a system state with a scenario of the second data provider" ) {
        Execution::Engine sourceEngine(model);
        sourceEngine.run(dataProvider->createScenario());
        REQUIRE_THROWS_AS( engine.initializeSystemState(otherDataProvider->createScenario(), sourceEngine.getSystemState()), std::invalid_argument );
      }
    }
  }
}

SCENARIO( "Outcome of a run", "[execution][process][outcome]" ) {
  auto model = std::make_shared<const Model::Model>("tests/execution/timer/Timer.bpmn");
  std::string csv =
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "Instance_1; Process_1; trigger := 10\n"
  ;

  GIVEN( "A single instance whose timer is triggered at time 10" ) {
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::OutcomeSentinel sentinel;
    sentinel.subscribe(&engine);

    WHEN( "The run ends once nothing is left to do" ) {
      engine.run(dataProvider->createScenario());

      THEN( "The run is completed" ) {
        REQUIRE( sentinel.getOutcome() == Execution::Outcome::COMPLETED );
      }
    }

    WHEN( "The run ends at the end time 5, with the instance still running" ) {
      dataProvider->setEndTime(5);
      engine.run(dataProvider->createScenario());

      THEN( "The run is terminated" ) {
        REQUIRE( sentinel.getOutcome() == Execution::Outcome::TERMINATED );
      }
    }
  }
}

SCENARIO( "Two instances of which one fails", "[execution][process]" ) {
  const std::string modelFile = "tests/execution/process/Process_with_deadline.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance missing its deadline at time 1 and an instance completing at time 5" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_1; Process_1; duration := 1\n"
      "Instance_1; Process_1; deadline := 0\n"
      "Instance_2; Process_1; timestamp := 0\n"
      "Instance_2; Process_1; duration := 5\n"
      "Instance_2; Process_1; deadline := 10\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      Execution::OutcomeSentinel sentinel;
      sentinel.subscribe(&engine);
      engine.run(std::move(scenario));

      // the position in the log of the first entry matching the given object
      auto position = [&recorder](const nlohmann::ordered_json& include) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( !recorder.log[i].is_object() ) {
            continue;
          }
          bool matches = true;
          for ( auto& [key, value] : include.items() ) {
            if ( !recorder.log[i].contains(key) || recorder.log[i][key] != value ) {
              matches = false;
              break;
            }
          }
          if ( matches ) {
            return i;
          }
        }
        return recorder.log.size();
      };

      THEN( "The instance missing its deadline fails" ) {
        auto processLog = recorder.find(nlohmann::json{{"instanceId","Instance_1"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.size() > 0 );
        REQUIRE( processLog.back()["state"] == "FAILED" );
      }
      THEN( "The other instance completes after the failure" ) {
        auto processLog = recorder.find(nlohmann::json{{"instanceId","Instance_2"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.size() == 6 );
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[4]["state"] == "COMPLETED" );
        REQUIRE( processLog[5]["state"] == "DONE" );
        REQUIRE( processLog[5]["status"]["timestamp"] == 5 );

        auto failure = position(nlohmann::ordered_json{{"instanceId","Instance_1"},{"state","FAILED"}});
        auto completion = position(nlohmann::ordered_json{{"instanceId","Instance_2"},{"nodeId","Activity_1"},{"state","COMPLETED"}});
        REQUIRE( failure < recorder.log.size() );
        REQUIRE( completion < recorder.log.size() );
        REQUIRE( failure < completion );
      }
      THEN( "The outcome of the run is a failure" ) {
        REQUIRE( sentinel.getOutcome() == Execution::Outcome::FAILED );
      }
    }
  }
}
