#include "prelude.h"

SCENARIO( "Intermediate escalation", "[execution][escalation]" ) {
  const std::string modelFile = "tests/execution/escalationevent/Uncaught_escalation.bpmn";
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
      auto tokenLog = recorder.find(nlohmann::json{}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
      THEN( "The token log has exactly 24 entries" ) {
        REQUIRE( tokenLog.size() == 24 );
      }
      THEN( "The dump of each entry of the token log is correct" ) {
        auto escalationEventLog = recorder.find(nlohmann::json{{"nodeId","EscalationEvent_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( escalationEventLog[0]["status"]["timestamp"] == 1.0);
        REQUIRE( escalationEventLog[0]["state"] == "ARRIVED" );
        REQUIRE( escalationEventLog[1]["state"] == "ENTERED" );
        REQUIRE( escalationEventLog[2]["state"] == "DEPARTED" );

        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog[0]["state"] == "CREATED" );
        REQUIRE( processLog[1]["state"] == "READY" );
        REQUIRE( processLog[2]["state"] == "ENTERED" );
        REQUIRE( processLog[3]["state"] == "BUSY" );
        REQUIRE( processLog[3]["status"]["timestamp"] == 0.0);
        // the escalation passing the process token leaves its status unchanged
        REQUIRE( processLog[4]["state"] == "COMPLETED" );
        REQUIRE( processLog[4]["status"]["timestamp"] == 1.0);
        REQUIRE( processLog[5]["state"] == "DONE" );
        REQUIRE( processLog.size() == 6 );
      }
    }
  }
}

SCENARIO( "Escalations carrying a content object", "[execution][escalation]" ) {
  GIVEN( "A named escalation thrown in a subprocess with boundary events catching it and every escalation" ) {
    const std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    auto model = std::make_shared<const Model::Model>("tests/execution/escalationevent/Named_escalation.bpmn");
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);
    engine.run(dataProvider->createScenario());
    THEN( "The boundary event naming the escalation catches it, reads its content and keeps its own status" ) {
      auto lateLog = recorder.find(nlohmann::json{{"nodeId","EscalationBoundaryEvent_Late"},{"state","COMPLETED"}});
      REQUIRE( lateLog.size() == 1 );
      REQUIRE( lateLog[0]["status"]["seen"] == 2 );
      REQUIRE( lateLog[0]["status"]["level"] == 3 );
      REQUIRE( recorder.find(nlohmann::json{{"nodeId","EscalationBoundaryEvent_All"},{"state","COMPLETED"}}).empty() );
    }
    THEN( "The status of the subprocess the escalation passes is not changed by it" ) {
      auto busyLog = recorder.find(nlohmann::json{{"nodeId","SubProcess_1"},{"state","BUSY"}});
      REQUIRE( busyLog.size() == 1 );
      REQUIRE( busyLog[0]["status"]["level"] == 3 );
    }
  }
}
