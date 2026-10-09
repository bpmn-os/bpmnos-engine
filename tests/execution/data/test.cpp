#include "prelude.h"

SCENARIO( "Task with expression operator", "[data]" ) {
  const std::string modelFile = "tests/execution/data/Data.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with an input value" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; data1 := 8\n"
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
      THEN( "The status updates are correct" ) {
        auto entryLog = recorder.find(nlohmann::json{{"nodeId", "SubProcess_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["data"]["data1"] == 8 );
        REQUIRE( entryLog[0]["data"]["data2"] == 0 );
        REQUIRE( entryLog[0]["data"]["data3"] == "mydata" );
        REQUIRE( entryLog[0]["data"]["data4"] == true );

        auto completionLog = recorder.find(nlohmann::json{{"nodeId", "SubProcess_1"},{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["data"]["data1"] == 42 );
        REQUIRE( completionLog[0]["data"]["data2"] == 0 );
        REQUIRE( completionLog[0]["data"]["data3"] == "mydata" );
        REQUIRE( completionLog[0]["data"]["data4"] == false );
      }
    }
  }
}
