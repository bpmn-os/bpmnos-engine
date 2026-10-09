#include "prelude.h"

SCENARIO( "Simple process with timer", "[execution][timer]" ) {
  const std::string modelFile = "tests/execution/timer/Timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started with a given trigger" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; trigger := 10\n"
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
      THEN( "The timer is triggered at the given time" ) {
        auto timerLog =recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state", "COMPLETED"}});
        REQUIRE( timerLog.front()["status"]["timestamp"] == 10.0 ); 
      }
    }
  }
}
