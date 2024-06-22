SCENARIO( "Empty executable subprocess", "[execution][subprocess]" ) {
  const std::string modelFile = "tests/execution/subprocess/Empty_executable_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
//      Execution::DeterministicTaskCompletion completionHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
//      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
      THEN( "The token log has exactly 16 entries" ) {
        REQUIRE( tokenLog.size() == 16 );
      }
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );

        auto startLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( startLog[0]["state"] == "ENTERED" );
        REQUIRE( startLog[1]["state"] == "DEPARTED" );

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
}

SCENARIO( "Trivial executable subprocess", "[execution][subprocess]" ) {
  const std::string modelFile = "tests/execution/subprocess/Trivial_executable_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
//      Execution::DeterministicTaskCompletion completionHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
//      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
      THEN( "The token log has exactly 18 entries" ) {
        REQUIRE( tokenLog.size() == 18 );
      }
      THEN( "The dump of each entry of the token log is correct" ) {
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "ENTERED" );
        REQUIRE( processLog[1]["state"] == "BUSY" );
        REQUIRE( processLog[2]["state"] == "COMPLETED" );
        REQUIRE( processLog[3]["state"] == "DONE" );

        auto startEvent1Log = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( startEvent1Log[0]["state"] == "ENTERED" );
        REQUIRE( startEvent1Log[1]["state"] == "DEPARTED" );

        auto startEvent2Log = recorder.find(nlohmann::json{{"nodeId","StartEvent_2" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( startEvent2Log[0]["state"] == "ENTERED" );
        REQUIRE( startEvent2Log[1]["state"] == "DONE" );

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
}

SCENARIO( "Constrained executable process", "[execution][subprocess]" ) {
  const std::string modelFile = "tests/execution/subprocess/Constrained_executable_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {
    WHEN( "The engine is started at time 0" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Timestamp,0\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The subprocess is traversed without failure" ) {
        auto subprocessLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( subprocessLog[0]["state"] == "ARRIVED" );
        REQUIRE( subprocessLog[1]["state"] == "READY" );
        REQUIRE( subprocessLog[2]["state"] == "ENTERED" );
        REQUIRE( subprocessLog[3]["state"] == "BUSY" );
        REQUIRE( subprocessLog[4]["state"] == "COMPLETED" );
        REQUIRE( subprocessLog[5]["state"] == "EXITING" );
        REQUIRE( subprocessLog[6]["state"] == "DEPARTED" );
      }
    }

    WHEN( "The engine is started at time 2" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Timestamp,2\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The subprocess fails after entry" ) {
        auto subprocessLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( subprocessLog[0]["state"] == "ARRIVED" );
        REQUIRE( subprocessLog[1]["state"] == "READY" );
        REQUIRE( subprocessLog[2]["state"] == "ENTERED" );
        REQUIRE( subprocessLog[3]["state"] == "FAILED" );
      }
    }

    WHEN( "The engine is started at time 1" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Timestamp,1\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The subprocess fails when exiting" ) {
        auto subprocessLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( subprocessLog[0]["state"] == "ARRIVED" );
        REQUIRE( subprocessLog[1]["state"] == "READY" );
        REQUIRE( subprocessLog[2]["state"] == "ENTERED" );
        REQUIRE( subprocessLog[3]["state"] == "BUSY" );
        REQUIRE( subprocessLog[4]["state"] == "FAILING" );
        REQUIRE( subprocessLog[5]["state"] == "FAILED" );
        
        auto taskLog = recorder.find(nlohmann::json{{"nodeId","Activity_2" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( taskLog[0]["state"] == "ARRIVED" );
        REQUIRE( taskLog[1]["state"] == "READY" );
        REQUIRE( taskLog[2]["state"] == "ENTERED" );
        REQUIRE( taskLog[3]["state"] == "BUSY" );
        REQUIRE( taskLog[4]["state"] == "COMPLETED" );
        REQUIRE( taskLog[5]["state"] == "EXITING" );
        REQUIRE( taskLog[6]["state"] == "FAILED" );

      }
    }
  }
}

