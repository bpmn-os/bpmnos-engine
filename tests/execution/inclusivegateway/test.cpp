#include "prelude.h"

SCENARIO( "Inclusive gateway with default flow", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_gateway_with_default_flow.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token departs along the default flow" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_C"},{"state","DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_A"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_B"}}).empty() );
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.back()["state"] == "DONE" );
      }
    }
  }

  GIVEN( "A single instance starting at time 1" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token departs along each sequence flow whose condition is satisfied" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_A"},{"state","DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_B"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_C"}}).empty() );
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.back()["state"] == "DONE" );
      }
    }
  }

  GIVEN( "A single instance starting at time 2" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 2\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token departs along each sequence flow whose condition is satisfied" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_A"},{"state","DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_B"},{"state","DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_C"}}).empty() );
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.back()["state"] == "DONE" );
      }
    }
  }
}

SCENARIO( "Inclusive gateway without default flow", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_gateway_without_default_flow.bpmn";
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
      THEN( "The run is aborted, as no condition of a sequence flow out of the gateway is satisfied" ) {
        REQUIRE_THROWS_WITH(
          engine.run(std::move(scenario)),
          "StateMachine: no condition of a sequence flow out of inclusive gateway 'Gateway_1' is satisfied, and it has no default flow"
        );
      }
    }
  }
}
