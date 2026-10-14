#include "prelude.h"

SCENARIO( "Operators of throw events", "[execution][eventoperators]" ) {
  const std::string modelFile = "tests/execution/eventoperators/Throw_event_operators.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

      THEN( "The untyped start event applies its operators when it is passed" ) {
        auto startLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1"},{"state","DEPARTED"}});
        REQUIRE( startLog.size() == 1 );
        REQUIRE( startLog[0]["status"]["started"] == 1 );
      }
      THEN( "The escalation throw event applies its operators before it throws, without becoming busy" ) {
        auto escalationLog = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_1"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationLog.size() == 3 );
        REQUIRE( escalationLog[0]["state"] == "ARRIVED" );
        REQUIRE( escalationLog[1]["state"] == "ENTERED" );
        REQUIRE( escalationLog[2]["state"] == "DEPARTED" );
        REQUIRE( escalationLog[2]["status"]["escalated"] == 2 );
      }
      THEN( "The untyped end event applies its operators when it is reached, without becoming busy" ) {
        auto endLog = recorder.find(nlohmann::json{{"nodeId","EndEvent_1"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( endLog.size() == 3 );
        REQUIRE( endLog[0]["state"] == "ARRIVED" );
        REQUIRE( endLog[1]["state"] == "ENTERED" );
        REQUIRE( endLog[2]["state"] == "DONE" );
        REQUIRE( endLog[2]["status"]["finished"] == 3 );
      }
    }
  }
}

SCENARIO( "Operators of catch events", "[execution][eventoperators]" ) {
  const std::string modelFile = "tests/execution/eventoperators/Catch_event_operators.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
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
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

      THEN( "The timer event applies its operators when it is triggered" ) {
        auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","DEPARTED"}});
        REQUIRE( timerLog.size() == 1 );
        REQUIRE( timerLog[0]["status"]["timestamp"] == 5 );
        REQUIRE( timerLog[0]["status"]["fired"] == 5 );
      }
      THEN( "The conditional event applies its operators when its conditions hold" ) {
        auto conditionalLog = recorder.find(nlohmann::json{{"nodeId","ConditionalEvent_1"},{"state","DEPARTED"}});
        REQUIRE( conditionalLog.size() == 1 );
        REQUIRE( conditionalLog[0]["status"]["checked"] == 6 );
      }
    }
  }
}

SCENARIO( "Operators of boundary events and of an error end event", "[execution][eventoperators]" ) {
  const std::string modelFile = "tests/execution/eventoperators/Boundary_event_operators.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

      THEN( "The escalation throw event applies its operators before it throws" ) {
        auto escalationLog = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_1"},{"state","DEPARTED"}});
        REQUIRE( escalationLog.size() == 1 );
        REQUIRE( escalationLog[0]["status"]["escalated"] == 1 );
      }
      THEN( "The escalation boundary event applies its operators when it is triggered" ) {
        auto boundaryLog = recorder.find(nlohmann::json{{"nodeId","EscalationBoundaryEvent_1"},{"state","DEPARTED"}});
        REQUIRE( boundaryLog.size() == 1 );
        REQUIRE( boundaryLog[0]["status"]["escalation_handled"] == 1 );
      }
      THEN( "The error end event applies its operators before it fails" ) {
        auto errorLog = recorder.find(nlohmann::json{{"nodeId","ErrorEndEvent_1"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( errorLog.size() == 3 );
        REQUIRE( errorLog[0]["state"] == "ARRIVED" );
        REQUIRE( errorLog[1]["state"] == "ENTERED" );
        REQUIRE( errorLog[2]["state"] == "FAILED" );
        REQUIRE( errorLog[2]["status"]["cause"] == 7 );
      }
      THEN( "The error boundary event applies its operators when it is triggered" ) {
        auto boundaryLog = recorder.find(nlohmann::json{{"nodeId","ErrorBoundaryEvent_1"},{"state","DEPARTED"}});
        REQUIRE( boundaryLog.size() == 1 );
        REQUIRE( boundaryLog[0]["status"]["error_handled"] == 1 );
      }
    }
  }
}

SCENARIO( "Operators of message events", "[execution][eventoperators]" ) {
  const std::string modelFile = "tests/execution/eventoperators/Message_event_operators.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

      THEN( "The message end event applies its operators before the message is built" ) {
        auto messageLog = recorder.find(nlohmann::json{{"origin","MessageThrowEvent_1"},{"state","CREATED"}});
        REQUIRE( messageLog.size() == 1 );
        REQUIRE( messageLog[0]["content"]["Payload"] == "42" );
      }
      THEN( "The message end event does not become busy" ) {
        auto throwLog = recorder.find(nlohmann::json{{"nodeId","MessageThrowEvent_1"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( throwLog.size() == 3 );
        REQUIRE( throwLog[0]["state"] == "ARRIVED" );
        REQUIRE( throwLog[1]["state"] == "ENTERED" );
        REQUIRE( throwLog[2]["state"] == "DONE" );
      }
      THEN( "The message catch event applies its operators after the content is received" ) {
        auto catchLog = recorder.find(nlohmann::json{{"nodeId","MessageCatchEvent_2"},{"state","DEPARTED"}});
        REQUIRE( catchLog.size() == 1 );
        REQUIRE( catchLog[0]["status"]["received"] == 42 );
        REQUIRE( catchLog[0]["status"]["doubled"] == 84 );
      }
    }
  }
}

SCENARIO( "Operators of signal events", "[execution][eventoperators]" ) {
  const std::string modelFile = "tests/execution/eventoperators/Signal_event_operators.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A recipient waiting before the signal is thrown" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n"
      "Instance_2; Process_2; timestamp := 0\n"
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
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

      THEN( "The signal throw event applies its operators before the signal is built" ) {
        auto signalLog = recorder.find(nlohmann::json{{"name","Signal"}});
        REQUIRE( signalLog.size() == 1 );
        REQUIRE( signalLog[0]["content"]["Payload"] == 42 );
      }
      THEN( "The signal catch event applies its operators after the content is received" ) {
        auto catchLog = recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state","DEPARTED"}});
        REQUIRE( catchLog.size() == 1 );
        REQUIRE( catchLog[0]["status"]["received"] == 42 );
        REQUIRE( catchLog[0]["status"]["doubled"] == 84 );
      }
    }
  }
}

SCENARIO( "Operators of start events", "[execution][eventoperators]" ) {
  const std::string modelFile = "tests/execution/eventoperators/Start_event_operators.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

      THEN( "The start event of the process applies its operators to the initial values of the process" ) {
        auto startLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_1"},{"state","DEPARTED"}});
        REQUIRE( startLog.size() == 1 );
        REQUIRE( startLog[0]["status"]["value"] == 6 );
      }
      THEN( "The start event of the subprocess applies its operators to the initial values of the subprocess" ) {
        auto startLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_2"},{"state","DEPARTED"}});
        REQUIRE( startLog.size() == 1 );
        REQUIRE( startLog[0]["status"]["inner"] == 7 );
      }
    }
  }
}

SCENARIO( "Operators of a message catch event in the evaluation of a delivery", "[execution][eventoperators]" ) {
  const std::string modelFile = "tests/execution/eventoperators/Message_catch_operator_objective.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two offers and a recipient whose objective its catch event's operator computes" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Sender_1; Process_1; payload := 1\n"
      "Sender_2; Process_1; payload := 5\n"
      "Recipient_1; Process_2;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with the greedy controller" ) {
      Execution::Engine engine(model);
      auto evaluator = std::make_shared<Execution::LocalEvaluator>();
      Execution::GreedyController controller(evaluator);
      controller.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

      THEN( "The delivery the operator values highest is chosen" ) {
        auto catchLog = recorder.find(nlohmann::json{{"nodeId","MessageCatchEvent_2"},{"state","DEPARTED"}});
        REQUIRE( catchLog.size() == 1 );
        REQUIRE( catchLog[0]["status"]["received"] == 5 );
        REQUIRE( catchLog[0]["status"]["score"] == 5 );
      }
    }
  }
}

SCENARIO( "Event operator modifying the timestamp", "[execution][eventoperators]" ) {
  const std::string modelFile = "tests/execution/eventoperators/Event_operator_modifying_timestamp.bpmn";
  THEN( "The model is refused" ) {
    REQUIRE_THROWS_WITH( Model::Model(modelFile), Catch::Matchers::ContainsSubstring("operators of event 'EndEvent_1' must not modify timestamp") );
  }
}

SCENARIO( "Operators of scopes", "[execution][eventoperators]" ) {
  THEN( "A process with operators is refused" ) {
    REQUIRE_THROWS_WITH( Model::Model("tests/execution/eventoperators/Process_with_operators.bpmn"), Catch::Matchers::ContainsSubstring("scope 'Process_1' must not have operators") );
  }
  THEN( "A subprocess with operators is refused" ) {
    REQUIRE_THROWS_WITH( Model::Model("tests/execution/eventoperators/Subprocess_with_operators.bpmn"), Catch::Matchers::ContainsSubstring("scope 'Activity_1' must not have operators") );
  }
  THEN( "An event subprocess with operators is refused" ) {
    REQUIRE_THROWS_WITH( Model::Model("tests/execution/eventoperators/Event_subprocess_with_operators.bpmn"), Catch::Matchers::ContainsSubstring("scope 'EventSubProcess_1' must not have operators") );
  }
}
