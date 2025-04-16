SCENARIO( "Knapsack problem", "[cpcontroller][knapsack_problem]" ) {
  const std::string modelFile = "examples/knapsack_problem/Knapsack_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One knapsack and three items" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "KnapsackProcess;Knapsack1;Capacity;40\n"
      "ItemProcess;Item1;Weight;20\n"
      "ItemProcess;Item1;Value;100\n"
      "ItemProcess;Item2;Weight;15\n"
      "ItemProcess;Item2;Value;50\n"
      "ItemProcess;Item3;Weight;22\n"
      "ItemProcess;Item3;Value;120\n"
    ;
    
/*
    WHEN( "The engine is started with the default seed" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
 
      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      [[maybe_unused]] auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); 
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
//      Execution::Recorder recorder;
      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);

//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors:\n" << solution.errors() << std::endl;
      THEN( "The solution is termiates" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( !terminationLog.empty() );
//        REQUIRE( solution.complete() );
//        REQUIRE( solution.errors().empty() );
//        REQUIRE( solution.getObjectiveValue().has_value() );
//        REQUIRE( solution.getObjectiveValue().value() == engine.getSystemState()->getObjective() );
      }
    }
*/
    WHEN( "The engine is started with a suitable partial seed" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
 
//      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( Execution::CPSeed::defaultSeed( {63,64,65,66,79,80,81,82,95,96,97,98}, controller.getVertices().size() ) );

      [[maybe_unused]] auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); 
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
//      Execution::Recorder recorder;
      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);

std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
std::cerr << "Solution:\n" << solution.stringify() << std::endl;
std::cerr << "Errors:\n" << solution.errors() << std::endl;
      THEN( "The solution is completes without errors" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );
        REQUIRE( solution.complete() );
        REQUIRE( solution.errors().empty() );
        REQUIRE( solution.getObjectiveValue().has_value() );
        REQUIRE( solution.getObjectiveValue().value() == engine.getSystemState()->getObjective() );
      }
    }
  }
};

