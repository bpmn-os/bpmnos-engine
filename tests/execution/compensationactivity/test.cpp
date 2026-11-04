#include "prelude.h"

SCENARIO( "Compensate throw event without compensations", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/No_compensation.bpmn";
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
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateThrowEvent_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Task with unused compensation task", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/Unused_compensation_task.bpmn";
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
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "EndEvent_1" );

        auto busyLog = recorder.find(nlohmann::json{{"state", "BUSY"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( busyLog[0]["nodeId"] == nullptr );
        REQUIRE( busyLog[1]["nodeId"] == "Activity_1" );
        REQUIRE( busyLog[2]["nodeId"] == "CompensateBoundaryEvent_1" );

        auto compensateBoundaryEventLog = recorder.find(nlohmann::json{{"nodeId","CompensateBoundaryEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( compensateBoundaryEventLog[0]["state"] == "CREATED" );
        REQUIRE( compensateBoundaryEventLog[1]["state"] == "ENTERED" );
        REQUIRE( compensateBoundaryEventLog[2]["state"] == "BUSY" );
        // the compensation can no longer be triggered when the instance completes
        REQUIRE( compensateBoundaryEventLog[3]["state"] == "WITHDRAWN" );
        REQUIRE( compensateBoundaryEventLog.size() == 4 );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Task with compensation task", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/Compensation_task.bpmn";
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
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[5]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Task with compensation triggered by error", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/Compensation_triggered_by_error.bpmn";
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
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "ErrorEndEvent_1" );
        REQUIRE( entryLog[5]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensationActivity_1" );

        auto failedLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failedLog[0]["nodeId"] == "ErrorEndEvent_1" );
        REQUIRE( failedLog[1]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Task with compensation subprocess", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/Compensation_subprocess.bpmn";
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
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[5]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == "StartEvent_2" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[4]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[5]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Named task with compensation task", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/Named_compensation.bpmn";
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
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[5]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Two compensations triggered in reverse order", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/Two_compensations_triggered.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Compensations execute in reverse order (Activity_2 first, then Activity_1)" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[3]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( entryLog[4]["nodeId"] == "Activity_2" );
        REQUIRE( entryLog[5]["nodeId"] == "CompensateBoundaryEvent_2" );
        REQUIRE( entryLog[6]["nodeId"] == "CompensateEndEvent_1" );
        REQUIRE( entryLog[7]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( entryLog[8]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "Activity_2" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensateBoundaryEvent_2" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( completionLog[4]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[5]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[6]["nodeId"] == "CompensateEndEvent_1" );
        REQUIRE( completionLog[7]["nodeId"] == nullptr );
      }
    }
  }
}

SCENARIO( "Values of a compensation activity", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/Compensation_values.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  auto model = std::make_shared<const Model::Model>(modelFile);

  auto run = [&](std::shared_ptr<Execution::DataProvider> dataProvider, const std::string& state = "") {
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    entryHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);
    engine.run(dataProvider->createScenario());
    if ( state.empty() ) {
      // the tokens only, not the ready event
      return recorder.find(nlohmann::json{{"nodeId","CompensationActivity_1"},{"state",nullptr}});
    }
    return recorder.find(nlohmann::json{{"nodeId","CompensationActivity_1"},{"state",state}});
  };

  GIVEN( "Instance data giving a value to an attribute of the compensation activity" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; CompensationActivity_1; given := 2\n"
    ;
    auto log = run(std::make_shared<Execution::StaticDataProvider>(model, csv));
    THEN( "The token arrives, becomes ready and enters when compensation is triggered" ) {
      REQUIRE( log.size() >= 3 );
      REQUIRE( log[0]["state"] == "ARRIVED" );
      REQUIRE( log[1]["state"] == "READY" );
      REQUIRE( log[2]["state"] == "ENTERED" );
    }
    THEN( "The compensation activity holds the status of the completion, the data and the time of the trigger, and the given value" ) {
      auto& entered = log[2];
      REQUIRE( entered["status"]["timestamp"] == 10 );
      REQUIRE( entered["status"]["level"] == 5 );
      REQUIRE( entered["status"]["seen_level"] == 5 );
      REQUIRE( entered["status"]["seen_stock"] == 7 );
      REQUIRE( entered["status"]["seen_time"] == 10 );
      REQUIRE( entered["status"]["given"] == 2 );
      REQUIRE( entered["status"]["sampled"].is_null() );
    }
  }

  const std::string header = "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n";
  const std::string process = "Instance_1; Process_1; timestamp := 0; ; ;\n";

  GIVEN( "A ready expression of the compensation activity" ) {
    auto enteredLog = run(std::make_shared<Execution::StochasticDataProvider>(model, header + process +
      "Instance_1; CompensationActivity_1; given := 2; ; ;\n"
      "Instance_1; CompensationActivity_1; ; ; sampled := given + 1;\n"
    ), "ENTERED");
    THEN( "The ready value is applied over the initial values" ) {
      REQUIRE( enteredLog.size() == 1 );
      REQUIRE( enteredLog[0]["status"]["timestamp"] == 10 );
      REQUIRE( enteredLog[0]["status"]["given"] == 2 );
      REQUIRE( enteredLog[0]["status"]["sampled"] == 3 );
    }
  }

  GIVEN( "A ready expression assigning the timestamp of the compensation activity" ) {
    THEN( "The data provider refuses it" ) {
      REQUIRE_THROWS_WITH( std::make_shared<Execution::StochasticDataProvider>(model, header + process +
        "Instance_1; CompensationActivity_1; ; ; timestamp := timestamp + 1;\n"
      ), Catch::Matchers::ContainsSubstring("must not assign the timestamp") );
    }
  }

  GIVEN( "A disclosure of the compensation activity" ) {
    THEN( "The stochastic data provider refuses it" ) {
      REQUIRE_THROWS_WITH( std::make_shared<Execution::StochasticDataProvider>(model, header + process +
        "Instance_1; CompensationActivity_1; given := 2; 5; ;\n"
      ), Catch::Matchers::ContainsSubstring("no disclosure may be given for compensation activity") );
    }
    THEN( "The dynamic data provider refuses it" ) {
      REQUIRE_THROWS_WITH( std::make_shared<Execution::DynamicDataProvider>(model,
        "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
        "Instance_1; Process_1; timestamp := 0; 0\n"
        "Instance_1; CompensationActivity_1; given := 2; 5\n"
      ), Catch::Matchers::ContainsSubstring("no disclosure may be given for compensation activity") );
    }
  }
}

SCENARIO( "Compensation after a failed completion check", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/Compensation_after_failed_completion.bpmn";
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
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The subprocess failing its completion check compensates the activity completed within it" ) {
        auto subProcessLog = recorder.find(nlohmann::json{{"nodeId","SubProcess_1"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( subProcessLog.size() >= 3 );
        REQUIRE( subProcessLog[subProcessLog.size()-3]["state"] == "COMPLETED" );
        REQUIRE( subProcessLog[subProcessLog.size()-2]["state"] == "FAILING" );
        REQUIRE( subProcessLog.back()["state"] == "FAILED" );

        auto compensationLog = recorder.find(nlohmann::json{{"nodeId","CompensationActivity_1"},{"state","COMPLETED"}});
        REQUIRE( compensationLog.size() == 1 );

        auto boundaryLog = recorder.find(nlohmann::json{{"nodeId","CompensateBoundaryEvent_1"},{"state","WITHDRAWN"}});
        REQUIRE( boundaryLog.empty() );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.back()["state"] == "FAILED" );
      }
    }
  }
}

SCENARIO( "Compensation failing at its compensate boundary event", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationactivity/Infeasible_compensate_boundary_event.bpmn";
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
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token at the compensate boundary event fails after its operators and the compensation activity does not run" ) {
        auto boundaryLog = recorder.find(nlohmann::json{{"nodeId","CompensateBoundaryEvent_1"}}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( boundaryLog.back()["state"] == "FAILED" );
        REQUIRE( boundaryLog.back()["status"]["x"] == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","CompensationActivity_1"}}).empty() );
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.back()["state"] == "FAILED" );
      }
    }
  }
}

