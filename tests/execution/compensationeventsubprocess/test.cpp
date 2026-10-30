#include "prelude.h"

SCENARIO( "Simple compensation event subprocess", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationeventsubprocess/Simple_compensation_event_subprocess.bpmn";
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
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( entryLog[3]["nodeId"] == "StartEvent_2" );
        REQUIRE( entryLog[4]["nodeId"] == "EndEvent_2" );
        REQUIRE( entryLog[5]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( entryLog[6]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[7]["nodeId"] == "EndEvent_3" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "StartEvent_2" );
        REQUIRE( completionLog[1]["nodeId"] == "SubProcess_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Recursive compensations", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationeventsubprocess/Recursive_compensations.bpmn";
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
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( entryLog[3]["nodeId"] == "StartEvent_2" );
        REQUIRE( entryLog[4]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[5]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( entryLog[6]["nodeId"] == "Activity_2" );
        REQUIRE( entryLog[7]["nodeId"] == "CompensateBoundaryEvent_2" );
        REQUIRE( entryLog[8]["nodeId"] == "EndEvent_2" );
        REQUIRE( entryLog[9]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( entryLog[10]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[11]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( entryLog[12]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( entryLog[13]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "StartEvent_2" );
        REQUIRE( completionLog[1]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[2]["nodeId"] == "Activity_2" );
        REQUIRE( completionLog[3]["nodeId"] == "SubProcess_1" );
        REQUIRE( completionLog[4]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( completionLog[5]["nodeId"] == "CompensateBoundaryEvent_2" );
        REQUIRE( completionLog[6]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( completionLog[7]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[8]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[9]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( completionLog[10]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[11]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Recursive named compensations", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationeventsubprocess/Recursive_named_compensations.bpmn";
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
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( entryLog[3]["nodeId"] == "StartEvent_2" );
        REQUIRE( entryLog[4]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[5]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( entryLog[6]["nodeId"] == "Activity_2" );
        REQUIRE( entryLog[7]["nodeId"] == "CompensateBoundaryEvent_2" );
        REQUIRE( entryLog[8]["nodeId"] == "EndEvent_2" );
        REQUIRE( entryLog[9]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( entryLog[10]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[11]["nodeId"] == "CompensateThrowEvent_2" );
        REQUIRE( entryLog[12]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( entryLog[13]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( entryLog[14]["nodeId"] == "CompensationActivity_2" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "StartEvent_2" );
        REQUIRE( completionLog[1]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[2]["nodeId"] == "Activity_2" );
        REQUIRE( completionLog[3]["nodeId"] == "SubProcess_1" );
        REQUIRE( completionLog[4]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( completionLog[5]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[6]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[7]["nodeId"] == "CompensateThrowEvent_2" );
        REQUIRE( completionLog[8]["nodeId"] == "CompensateBoundaryEvent_2" );
        REQUIRE( completionLog[9]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( completionLog[10]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( completionLog[11]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[12]["nodeId"] == nullptr );
     }
    }
  }
}

SCENARIO( "Compensation event subprocess of a subprocess with data", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationeventsubprocess/Compensation_event_subprocess_snapshot.bpmn";
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
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The event subprocess comes into being with the status and data of the subprocess at its completion" ) {
        auto completionLog = recorder.find(nlohmann::json{{"nodeId","CompensateStartEvent_1"},{"state","COMPLETED"}});
        REQUIRE( completionLog.size() == 1 );
        REQUIRE( completionLog[0]["status"]["timestamp"] == 10 );
        REQUIRE( completionLog[0]["status"]["seen_kept"] == 4 );
        REQUIRE( completionLog[0]["status"]["seen_stock"] == 0 );
        REQUIRE( completionLog[0]["status"]["created"] == 0 );
        REQUIRE( completionLog[0]["data"]["kept"] == 4 );
        REQUIRE( completionLog[0]["data"]["stock"] == 7 );
      }
    }
  }
}


SCENARIO( "Named compensation of a subprocess with a compensation event subprocess", "[execution][compensation]" ) {
  const std::string modelFile = "tests/execution/compensationeventsubprocess/Named_compensation_event_subprocess.bpmn";
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
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The compensation event subprocess of the named subprocess is triggered" ) {
        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}}, nlohmann::json{{"nodeId","StartEvent_1"}});
        REQUIRE( completionLog[0]["nodeId"] == "StartEvent_2" );
        REQUIRE( completionLog[1]["nodeId"] == "SubProcess_1" );
        REQUIRE( completionLog[2]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == nullptr );
      }
    }
  }
}
