#include "prelude.h"

SCENARIO( "Events of the dynamic data provider", "[data][dynamic]" ) {
  auto model = std::make_shared<const Model::Model>("tests/data/dynamicprovider/Executable_process.bpmn");

  GIVEN( "An instance disclosed at time 10 with timestamp 15, whose activity is disclosed at time 20" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 15; 5\n"
      "Instance_1; Process_1; x := 1; 10\n"
      "Instance_1; Activity_1; data := 5; 15\n"
      "Instance_1; Activity_1; y := 2; 20\n"
    ;
    auto dataProvider = std::make_shared<Execution::DynamicDataProvider>(model, csv);

    WHEN( "The engine runs on a scenario of the data provider" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The instance becomes known at time 10" ) {
        std::optional<double> time;
        for ( auto& entry : recorder.log ) {
          if ( entry.contains("event") && entry["event"] == "clocktick" ) {
            time = entry["time"].get<double>();
          }
          if ( entry.contains("event") && entry["event"] == "instantiation" ) {
            break;
          }
        }
        REQUIRE( time.has_value() );
        REQUIRE( time.value() == 10.0 );
      }
      THEN( "The process becomes ready at time 15" ) {
        auto readyLog = recorder.find(nlohmann::json{{"processId","Process_1"},{"state","READY"}}, nlohmann::json{{"nodeId",nullptr}});
        REQUIRE( readyLog.size() == 1 );
        REQUIRE( readyLog.front()["status"]["timestamp"] == 15.0 );
      }
      THEN( "The activity becomes ready at time 20 with the values disclosed" ) {
        auto arrivedLog = recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","ARRIVED"}});
        REQUIRE( arrivedLog.size() == 1 );
        REQUIRE( arrivedLog.front()["status"]["timestamp"] == 15.0 );
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","READY"}});
        REQUIRE( readyLog.size() == 1 );
        REQUIRE( readyLog.front()["status"]["timestamp"] == 20.0 );
        REQUIRE( readyLog.front()["status"]["y"] == 2 );
        REQUIRE( readyLog.front()["data"]["data"] == 5.0 );
      }
      THEN( "The run ends with a termination event" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
      }
    }
  }

  GIVEN( "A table without the column of disclosures" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 3\n"
    ;
    auto dataProvider = std::make_shared<Execution::DynamicDataProvider>(model, csv);

    WHEN( "The engine runs on a scenario of the data provider" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "Every value is disclosed at the start and the process becomes ready at time 3" ) {
        REQUIRE( recorder.log[1]["event"] == "instantiation" );
        auto readyLog = recorder.find(nlohmann::json{{"processId","Process_1"},{"state","READY"}}, nlohmann::json{{"nodeId",nullptr}});
        REQUIRE( readyLog.size() == 1 );
        REQUIRE( readyLog.front()["status"]["timestamp"] == 3.0 );
      }
    }
  }

  GIVEN( "A disclosure without initialization" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1;; 5\n"
    ;
    THEN( "The data provider rejects it" ) {
      REQUIRE_THROWS( Execution::DynamicDataProvider(model, csv) );
    }
  }
}
