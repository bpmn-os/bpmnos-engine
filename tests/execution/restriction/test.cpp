#include "prelude.h"

SCENARIO( "Restrictions inherited from enclosing scopes", "[execution][restriction]" ) {
  const std::string modelFile = "tests/execution/restriction/Inherited_restrictions.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A task nested in two subprocesses within a process, each scope restricting the level, and instances setting the level violating each restriction in turn or none" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; target := 1\n"
      "Instance_2; Process_1; target := 2\n"
      "Instance_3; Process_1; target := 3\n"
      "Instance_4; Process_1; target := 4\n"
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
      THEN( "The task fails where the level violates the restriction of the process or of either subprocess, and completes otherwise" ) {
        // the restrictions are checked once the task has completed, so that a failing task completes before it fails
        for ( std::string instanceId : { "Instance_1", "Instance_2", "Instance_3" } ) {
          REQUIRE( recorder.find(nlohmann::json{{"instanceId",instanceId},{"nodeId","Task_1"},{"state","FAILED"}}).size() == 1 );
        }
        REQUIRE( recorder.find(nlohmann::json{{"instanceId","Instance_4"},{"nodeId","Task_1"},{"state","COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"instanceId","Instance_4"},{"nodeId","Task_1"},{"state","FAILED"}}).empty() );
      }
    }
  }
}

SCENARIO( "Restrictions not inherited by an error event subprocess", "[execution][restriction]" ) {
  const std::string modelFile = "tests/execution/restriction/Error_event_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "An error event subprocess setting a level the process forbids, with a task setting a level the event subprocess forbids or not" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; target := 1\n"
      "Instance_2; Process_1; target := 2\n"
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
      THEN( "The restriction of the process does not apply within the event subprocess, whereas its own restriction does" ) {
        REQUIRE( recorder.find(nlohmann::json{{"instanceId","Instance_1"},{"nodeId","Task_1"},{"state","COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"instanceId","Instance_1"},{"nodeId","Task_1"},{"state","FAILED"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"instanceId","Instance_2"},{"nodeId","Task_1"},{"state","FAILED"}}).size() == 1 );
      }
    }
  }
}

SCENARIO( "Restrictions not inherited by a compensation activity", "[execution][restriction]" ) {
  const std::string modelFile = "tests/execution/restriction/Compensation_activity.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A compensation activity setting a level the process forbids" ) {

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
      THEN( "The compensation activity completes" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","CompensationActivity_1"},{"state","COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","CompensationActivity_1"},{"state","FAILED"}}).empty() );
      }
    }
  }
}
