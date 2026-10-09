#include "prelude.h"

SCENARIO( "Events of the stochastic data provider", "[data][stochastic]" ) {

  GIVEN( "A task with ready and completion expressions" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/task/Task_with_linear_expression.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1;;;;\n"
      "Instance_1; Activity_1;;; timestamp := triangular(5,5,5); timestamp := triangular(10,10,10)\n"
    ;
    auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(model, csv);

    WHEN( "The engine runs on a scenario of the data provider" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The task becomes ready and completes with the statuses the expressions give" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
        REQUIRE( activityLog[0]["state"] == "ARRIVED" );
        REQUIRE( activityLog[0]["status"]["timestamp"] == 0.0 );
        REQUIRE( activityLog[1]["state"] == "READY" );
        REQUIRE( activityLog[1]["status"]["timestamp"] == 5.0 );
        REQUIRE( activityLog[3]["state"] == "BUSY" );
        REQUIRE( activityLog[3]["status"]["timestamp"] == 6.0 );
        REQUIRE( activityLog[4]["state"] == "COMPLETED" );
        REQUIRE( activityLog[4]["status"]["timestamp"] == 10.0 );
      }
      THEN( "The run ends with a termination event" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
      }
    }
  }

  GIVEN( "An instance whose timestamp is uniformly distributed between 0 and 100" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/process/Empty_executable_process.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := uniform(0,100)\n"
    ;
    auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(model, csv, 42);

    auto readyTime = [&](unsigned int realisation) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario(realisation), 0);
      auto readyLog = recorder.find(nlohmann::json{{"processId","Process_1"},{"state","READY"}}, nlohmann::json{{"nodeId",nullptr}});
      REQUIRE( readyLog.size() == 1 );
      return readyLog.front()["status"]["timestamp"].get<double>();
    };

    THEN( "Scenarios of one realisation are equal and scenarios of different realisations differ" ) {
      REQUIRE( readyTime(0) == readyTime(0) );
      REQUIRE( readyTime(0) != readyTime(1) );
    }
  }

  GIVEN( "A completion expression for a node that is not a task" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/process/Empty_executable_process.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1;;;; timestamp := 1\n"
    ;
    THEN( "The data provider rejects it" ) {
      REQUIRE_THROWS( Execution::StochasticDataProvider(model, csv) );
    }
  }

  GIVEN( "A ready expression for an event subprocess" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/eventsubprocess/Non-interrupting_escalation.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1;;;;\n"
      "Instance_1; EventSubProcess_1;;; timestamp := 1;\n"
    ;
    THEN( "The data provider rejects it" ) {
      REQUIRE_THROWS( Execution::StochasticDataProvider(model, csv) );
    }
  }
}
