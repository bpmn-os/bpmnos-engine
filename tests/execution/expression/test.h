SCENARIO( "Linear expression", "[execution][expression]" ) {
  const std::string modelFile = "tests/execution/expression/linearExpression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A trivial instance with assignment expression z := 3*x + 5*y" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; x := 8\n"
        "Instance_1; Process_1; y := 15\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
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
      auto tokenLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }, {"state","ENTERED" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});

      THEN( "The token log has entries" ) {
        REQUIRE( tokenLog.size() );
      }
      THEN( "The first entry of the token log has the correct data" ) {
        REQUIRE( tokenLog.front()["instanceId"] == "Instance_1");
        REQUIRE( tokenLog.front()["processId"] == "Process_1");
        REQUIRE( tokenLog.front()["state"] == "ENTERED");
        REQUIRE( tokenLog.front()["data"]["instance"] == "Instance_1");
        REQUIRE( tokenLog.front()["status"]["x"] == 8);
        REQUIRE( tokenLog.front()["status"]["y"] == 15);
        REQUIRE( tokenLog.front()["status"]["z"] == 3*8 + 5*15);
      }
    }
  }
};

SCENARIO( "Divide assignment", "[execution][expression]" ) {
  const std::string modelFile = "tests/execution/expression/divideAssignment.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A trivial instance with divide assignment expression z /= 3*x + 5*y" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; x := 5\n"
        "Instance_1; Process_1; y := 3\n"
        "Instance_1; Process_1; z := 45\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
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
      auto startEventLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }, {"state","ENTERED" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});

      THEN( "The start event log has entries" ) {
        REQUIRE( startEventLog.size() );
      }
      THEN( "The first entry of the start event log has the correct data" ) {
        REQUIRE( startEventLog.front()["instanceId"] == "Instance_1");
        REQUIRE( startEventLog.front()["processId"] == "Process_1");
        REQUIRE( startEventLog.front()["state"] == "ENTERED");
        REQUIRE( startEventLog.front()["data"]["instance"] == "Instance_1");
        REQUIRE( startEventLog.front()["status"]["x"] == 5);
        REQUIRE( startEventLog.front()["status"]["y"] == 3);
        REQUIRE( startEventLog.front()["status"]["z"] == 45);
      }

      auto taskLog = recorder.find(nlohmann::json{{"nodeId","Task_1" }, {"state","COMPLETED" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});

      THEN( "The task log has entries" ) {
        REQUIRE( taskLog.size() );
      }
      THEN( "The first entry of the task log has the correct data" ) {
        REQUIRE( taskLog.front()["status"]["x"] == 5);
        REQUIRE( taskLog.front()["status"]["y"] == 3);
        REQUIRE( taskLog.front()["status"]["z"] == 45.0 / (3*5 + 5*3));
      }
    }
  }
};

SCENARIO( "String expression", "[execution][expression]" ) {
  const std::string modelFile = "tests/execution/expression/stringExpression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An expression result := name in {\"Peter\", example, \"Mary\"}" ) {
    WHEN( "The expression is executed with name = \"Mary\" and example = \"Paul\"" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; name := \"Joe\"\n"
        "Instance_1; Process_1; example := \"Paul\"\n"
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
      auto tokenLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }, {"state","ENTERED" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});

      THEN( "The token log has entries" ) {
        REQUIRE( tokenLog.size() );
      }
      THEN( "The first entry of the token log has the correct data" ) {
        REQUIRE( tokenLog.front()["instanceId"] == "Instance_1");
        REQUIRE( tokenLog.front()["processId"] == "Process_1");
        REQUIRE( tokenLog.front()["state"] == "ENTERED");
        REQUIRE( tokenLog.front()["data"]["instance"] == "Instance_1");
        REQUIRE( tokenLog.front()["status"]["name"] == "Joe");
        REQUIRE( tokenLog.front()["status"]["example"] == "Paul");
        REQUIRE( tokenLog.front()["status"]["result"] == false);
      }
    }
    WHEN( "The expression is executed with name = \"Mary\" and example = \"Paul\"" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; name := \"Mary\"\n"
        "Instance_1; Process_1; example := \"Paul\"\n"
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
      auto tokenLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }, {"state","ENTERED" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});

      THEN( "The token log has entries" ) {
        REQUIRE( tokenLog.size() );
      }
      THEN( "The first entry of the token log has the correct data" ) {
        REQUIRE( tokenLog.front()["instanceId"] == "Instance_1");
        REQUIRE( tokenLog.front()["processId"] == "Process_1");
        REQUIRE( tokenLog.front()["state"] == "ENTERED");
        REQUIRE( tokenLog.front()["data"]["instance"] == "Instance_1");
        REQUIRE( tokenLog.front()["status"]["name"] == "Mary");
        REQUIRE( tokenLog.front()["status"]["example"] == "Paul");
        REQUIRE( tokenLog.front()["status"]["result"] == true);
      }
    }
  }
};

SCENARIO( "Lookup table", "[execution][lookup]" ) {
  const std::string modelFile = "tests/execution/expression/lookupTable.bpmn";
  const std::vector<std::string> folders = { "tests/execution/expression" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "A trivial instance without parameters" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,folders,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
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
      auto tokenLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1" }, {"state","ENTERED" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});

      THEN( "The token log has entries" ) {
        REQUIRE( tokenLog.size() );
      }
      THEN( "The first entry of the token log has the correct data" ) {
        REQUIRE( tokenLog.front()["instanceId"] == "Instance_1");
        REQUIRE( tokenLog.front()["processId"] == "Process_1");
        REQUIRE( tokenLog.front()["state"] == "ENTERED");
        REQUIRE( tokenLog.front()["data"]["instance"] == "Instance_1");
        REQUIRE( tokenLog.front()["status"]["client"] == "Client3");
        REQUIRE( tokenLog.front()["status"]["costs"] == 3);
      }
    }
  }
};

