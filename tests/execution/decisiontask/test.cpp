#include "prelude.h"

SCENARIO( "Decision task with enumeration", "[execution][decisiontask]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with x=-2" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := -2\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      auto evaluator = std::make_shared<Execution::LocalEvaluator>();
      Execution::GreedyDispatcher<Execution::FirstChoice> choiceHandler(evaluator);
      entryHandler.connect(&engine);
      choiceHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);

      engine.run(std::move(scenario));

      THEN( "The choice made is correct" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" },{"state","COMPLETED"}});
        REQUIRE( activityLog[0]["status"]["choice"] == -2 );
      }
    }
  }

  GIVEN( "A single instance with x=4" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := 4\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      auto evaluator = std::make_shared<Execution::LocalEvaluator>();
      Execution::GreedyDispatcher<Execution::FirstChoice> choiceHandler(evaluator);
      entryHandler.connect(&engine);
      choiceHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);

      engine.run(std::move(scenario));

      THEN( "The choice made is correct" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" },{"state","COMPLETED"}});
        REQUIRE( activityLog[0]["status"]["choice"] == 3 );
      }
    }
  }
}

SCENARIO( "Decision task with bounds", "[execution][decisiontask]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_bounds.bpmn";
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
      auto evaluator = std::make_shared<Execution::LocalEvaluator>();
      Execution::GreedyDispatcher<Execution::FirstChoice> choiceHandler(evaluator);
      entryHandler.connect(&engine);
      choiceHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);

      engine.run(std::move(scenario));

      THEN( "The dump of each entry of the token log is correct" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" },{"state","COMPLETED"}});
        REQUIRE( activityLog[0]["status"]["choice"] == 0 );  // every integer between the bounds is an alternative, and 0 minimizes the square
      }
    }
  }

  GIVEN( "A stochastic instance with init, disclosure, and ready" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1;;;;\n"
      "Instance_1; Activity_1; square := triangular(0,0,0); triangular(5,5,5); square := triangular(2,2,2);\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      auto evaluator = std::make_shared<Execution::LocalEvaluator>();
      Execution::GreedyDispatcher<Execution::FirstChoice> choiceHandler(evaluator);
      entryHandler.connect(&engine);
      choiceHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);

      engine.run(std::move(scenario));

      THEN( "The ready expression is applied" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" },{"state","READY"}});
        REQUIRE( activityLog[0]["status"]["timestamp"] == 5 );
        REQUIRE( activityLog[0]["status"]["square"] == 2 );
      }
      THEN( "The choice is made" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" },{"state","COMPLETED"}});
        REQUIRE( activityLog[0]["status"]["choice"] == 0 );  // every integer between the bounds is an alternative, and 0 minimizes the square
      }
    }
  }

  GIVEN( "A stochastic instance with a completion expression for the decision task" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1;;;;\n"
      "Instance_1; Activity_1; square := triangular(0,0,0); triangular(5,5,5); square := triangular(2,2,2); square := 2 * choice\n"
    ;

    THEN( "The data provider rejects the completion expression" ) {
      REQUIRE_THROWS( Execution::StochasticDataProvider(std::make_shared<const Model::Model>(modelFile), csv) );
    }
  }
}

SCENARIO( "Decision task with continuous bounds", "[execution][decisiontask]" ) {
  // a decimal attribute chosen between bounds without a step may take every value of the interval
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_continuous_bounds.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with the first choice dispatcher without a threshold" ) {
      Execution::Engine engine(model);
      auto evaluator = std::make_shared<Execution::LocalEvaluator>();
      Execution::GreedyDispatcher<Execution::FirstChoice> choiceHandler(evaluator);
      choiceHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

      THEN( "The choice is bisected over the multiples of the precision of numbers and minimizes the square" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" },{"state","COMPLETED"}});
        REQUIRE( activityLog.size() == 1 );
        REQUIRE( activityLog[0]["status"]["choice"] > -2 );
        REQUIRE( activityLog[0]["status"]["choice"] <= 5 );
        REQUIRE( activityLog[0]["status"]["square"] == 0 );
      }
    }

    WHEN( "The engine is started with a dispatcher choosing at random" ) {
      Execution::Engine engine(model);
      Execution::RandomChoice choiceHandler;
      choiceHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));

      THEN( "A value of the interval is chosen" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" },{"state","COMPLETED"}});
        REQUIRE( activityLog.size() == 1 );
        REQUIRE( activityLog[0]["status"]["choice"] > -2 );
        REQUIRE( activityLog[0]["status"]["choice"] <= 5 );
      }
    }
  }
}
