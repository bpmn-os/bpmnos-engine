#include "prelude.h"

SCENARIO( "Loop task", "[execution][loopactivity]" ) {
  const std::string modelFile = "tests/execution/loopactivity/Loop_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {


    WHEN( "The engine is started with a maximum of 2 loops" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
        "Instance_1; LoopActivity_1; maximum := 2\n"
      ;

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The loop activity is performed twice" ) {
        auto entryLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 2 );
 
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "READY"}});
        REQUIRE( readyLog.size() == 2 );
      }
    }

    WHEN( "The engine is started with an irrelevant loop maximum" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
        "Instance_1; LoopActivity_1; maximum := 4\n"
      ;

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The loop activity is performed as long as the loop condition holds" ) {
        auto entryLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 3 );
 
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "READY"}});
        REQUIRE( readyLog.size() == 3 );
      }
    }
  }
}

SCENARIO( "Loop task with boundary event", "[execution][loopactivity]" ) {
  const std::string modelFile = "tests/execution/loopactivity/Loop_task_with_boundary_event.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with a maximum of 2 loops" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; LoopActivity_1; maximum := 2\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token at the boundary event is created once for both loops and withdrawn when the activity is left" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "ENTERED"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","BoundaryEvent_1"},{"state", "CREATED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","BoundaryEvent_1"},{"state", "WITHDRAWN"}}).size() == 1 );
      }
    }
  }
}

SCENARIO( "Loop subprocess", "[execution][loopactivity]" ) {
  const std::string modelFile = "tests/execution/loopactivity/Loop_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {


    WHEN( "The engine is started with a maximum of 2 loops" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
        "Instance_1; LoopActivity_1; maximum := 2\n"
      ;

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
    Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The loop activity is performed twice" ) {
        auto entryLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 2 );
 
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "READY"}});
        REQUIRE( readyLog.size() == 2 );
      }
    }

    WHEN( "The engine is started with an irrelevant loop maximum" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
        "Instance_1; LoopActivity_1; maximum := 4\n"
      ;

      auto model = std::make_shared<const Model::Model>(modelFile);
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      auto scenario = dataProvider->createScenario();

      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The loop activity is performed as long as the loop condition holds" ) {
        auto entryLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 3 );
 
        auto readyLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "READY"}});
        REQUIRE( readyLog.size() == 3 );
      }
    }
  }
}


SCENARIO( "Loop subprocess with operators", "[execution][loopactivity]" ) {
  const std::string modelFile = "tests/execution/loopactivity/Loop_subprocess_with_operators.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with a maximum of 3 loops" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; LoopActivity_1; maximum := 3\n"
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

      THEN( "The loop activity is entered three times" ) {
        auto entryLog = recorder.find(nlohmann::json{{"nodeId","LoopActivity_1"},{"state", "ENTERED"}});
        REQUIRE( entryLog.size() == 3 );
      }

      THEN( "The operators of the loop activity are applied once per iteration" ) {
        auto startLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( startLog.size() == 3 );
        REQUIRE( startLog[0]["globals"]["counter"] == 1 );
        REQUIRE( startLog[1]["globals"]["counter"] == 2 );
        REQUIRE( startLog[2]["globals"]["counter"] == 3 );
      }
    }
  }
}
