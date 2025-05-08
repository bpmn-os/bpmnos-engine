SCENARIO( "Bin packing problem", "[cpcontroller][bin_packing_problem]" ) {
  const std::string modelFile = "examples/bin_packing_problem/Bin_packing_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One bins and one item" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;Bins;1\n"
      ";;Items;1\n"
      "ItemProcess;Item1;Size;20.0\n"
      "BinProcess;Bin1;Capacity;40.0\n"
    ;

    WHEN( "The engine is started" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
 
      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
//      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); 
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      auto& solution = controller.createSolution();
      solution.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());

//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors:\n" << solution.errors() << std::endl;
      THEN( "The solution is complete and satisfies all constraints" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );
        REQUIRE( solution.complete() );
        REQUIRE( solution.errors().empty() );
        REQUIRE( solution.getObjectiveValue().has_value() );
        REQUIRE( solution.getObjectiveValue().value() == engine.getSystemState()->getObjective() );
      }
    }
  }

  GIVEN( "Two bins and one item" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;Bins;2\n"
      ";;Items;1\n"
      "ItemProcess;Item1;Size;20.0\n"
      "BinProcess;Bin1;Capacity;40.0\n"
      "BinProcess;Bin2;Capacity;40.0\n"
    ;

    WHEN( "The engine is started" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
 
      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
//      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); 
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      auto& solution = controller.createSolution();
      solution.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());

//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors:\n" << solution.errors() << std::endl;
      THEN( "The solution is complete and satisfies all constraints" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );
        REQUIRE( solution.complete() );
        REQUIRE( solution.errors().empty() );
        REQUIRE( solution.getObjectiveValue().has_value() );
        REQUIRE( solution.getObjectiveValue().value() == engine.getSystemState()->getObjective() );
      }
    }
  }

  GIVEN( "Three bins and three items" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;Bins;3\n"
      ";;Items;3\n"
      "ItemProcess;Item1;Size;20.0\n"
      "ItemProcess;Item2;Size;15.0\n"
      "ItemProcess;Item3;Size;22.0\n"
      "BinProcess;Bin1;Capacity;40.0\n"
      "BinProcess;Bin2;Capacity;40.0\n"
      "BinProcess;Bin3;Capacity;40.0\n"
    ;

    WHEN( "The engine is started" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
 
      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
//      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); 
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);

      auto& solution = controller.createSolution();
      solution.subscribe(&engine);
      engine.run(scenario.get());

//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors:\n" << solution.errors() << std::endl;
      THEN( "The solution is complete and satisfies all constraints" ) {
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

