SCENARIO( "Travelling salesperson problem", "[cpcontroller][travelling_salesperson_problem]" ) {
  const std::string modelFile = "examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/travelling_salesperson_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "A TSP with four locations" ) {
    WHEN( "The engine is started" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "TravellingSalesperson_Process;Instance1;Origin;\"Hamburg\"\n"
      "TravellingSalesperson_Process;Instance1;Locations;[\"Munich\",\"Berlin\",\"Cologne\"]\n"
    ;

      Model::StaticDataProvider dataProvider(modelFile,folders,csv);
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
};

