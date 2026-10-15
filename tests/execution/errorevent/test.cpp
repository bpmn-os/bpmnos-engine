#include "prelude.h"

SCENARIO( "Error end event", "[execution][error]" ) {
  const std::string modelFile = "tests/execution/errorevent/Simple_error.bpmn";
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
      dataProvider->setEndTime(0);
      engine.run(std::move(scenario));
      THEN( "The dump of each entry of the recorder log is correct" ) {
        auto errorEventLog = recorder.find(nlohmann::json{{"nodeId","ErrorEvent_1" }});
        REQUIRE( errorEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( errorEventLog[1]["state"] == "ENTERED" );
        REQUIRE( errorEventLog[2]["state"] == "FAILED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[4]["state"] == "FAILING" );
        REQUIRE( processLog[5]["state"] == "FAILED" );

/*
        size_t i=0;
        // process
        REQUIRE( recorder.log[i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"BUSY\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // start event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"StartEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"DEPARTED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // error event
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEvent_1\",\"sequenceFlowId\":\"Flow_0u4dkbp\",\"state\":\"ARRIVED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEvent_1\",\"state\":\"ENTERED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"nodeId\":\"ErrorEvent_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        // process
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILING\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
        REQUIRE( recorder.log[++i].dump() == "{\"processId\":\"Process_1\",\"instanceId\":\"Instance_1\",\"state\":\"FAILED\",\"status\":{\"timestamp\":0.0,\"instance\":\"Instance_1\"}}" );
*/
      }
    }
  }
}

SCENARIO( "Errors carrying a content object", "[execution][error]" ) {
  const std::string csv =
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "Instance_1; Process_1;\n"
  ;

  GIVEN( "A named error thrown in a subprocess with boundary events catching it, another error, and every error" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/errorevent/Named_error.bpmn");
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);
    engine.run(dataProvider->createScenario());
    THEN( "The boundary event naming the error catches it, reads its content and keeps its own status" ) {
      auto shortageLog = recorder.find(nlohmann::json{{"nodeId","ErrorBoundaryEvent_Shortage"},{"state","COMPLETED"}});
      REQUIRE( shortageLog.size() == 1 );
      REQUIRE( shortageLog[0]["status"]["seen"] == 4 );
      REQUIRE( shortageLog[0]["status"]["level"] == 3 );
      REQUIRE( recorder.find(nlohmann::json{{"nodeId","ErrorBoundaryEvent_All"},{"state","COMPLETED"}}).empty() );
      REQUIRE( recorder.find(nlohmann::json{{"nodeId","ErrorBoundaryEvent_Other"},{"state","COMPLETED"}}).empty() );
    }
  }

  GIVEN( "A subprocess failing by a violated restriction with boundary events catching a named error and every error" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/errorevent/Violated_restriction.bpmn");
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);
    engine.run(dataProvider->createScenario());
    THEN( "Only the boundary event catching every error catches the failure" ) {
      REQUIRE( recorder.find(nlohmann::json{{"nodeId","ErrorBoundaryEvent_All"},{"state","COMPLETED"}}).size() == 1 );
      REQUIRE( recorder.find(nlohmann::json{{"nodeId","ErrorBoundaryEvent_Shortage"},{"state","COMPLETED"}}).empty() );
      REQUIRE( recorder.find(nlohmann::json{{"nodeId","ErrorBoundaryEvent_Other"},{"state","COMPLETED"}}).empty() );
    }
  }

  GIVEN( "A named error thrown in a process with event subprocesses catching every error and the error named" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/errorevent/Named_error_event_subprocess.bpmn");
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);
    engine.run(dataProvider->createScenario());
    THEN( "The event subprocess naming the error is triggered, its start event reading the content and keeping its own status" ) {
      auto shortageLog = recorder.find(nlohmann::json{{"nodeId","ErrorStartEvent_Shortage"},{"state","COMPLETED"}});
      REQUIRE( shortageLog.size() == 1 );
      REQUIRE( shortageLog[0]["status"]["seen"] == 4 );
      REQUIRE( shortageLog[0]["status"]["level"] == 3 );
      REQUIRE( recorder.find(nlohmann::json{{"nodeId","ErrorStartEvent_All"},{"state","COMPLETED"}}).empty() );
    }
  }

  GIVEN( "A thrower and a catcher of an error stating different types" ) {
    THEN( "The model is refused" ) {
      REQUIRE_THROWS_WITH( Model::Model("tests/execution/errorevent/Disagreeing_error_types.bpmn"), Catch::Matchers::ContainsSubstring("must have the same type") );
    }
  }
}
