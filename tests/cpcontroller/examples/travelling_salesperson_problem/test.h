SCENARIO( "CPController with travelling salesperson problem", "[cpcontroller][travelling_salesperson_problem]" ) {
  const std::string modelFile = "examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/travelling_salesperson_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "A TSP with four locations" ) {
    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "TravellingSalesperson_Process;Instance1;Origin;\"Hamburg\"\n"
      "TravellingSalesperson_Process;Instance1;Locations;[\"Munich\",\"Berlin\",\"Cologne\"]\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,folders,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The observed solution is used to control execution" ) {
      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController greedyController( &flattenedGraph, &evaluator );

      Execution::Engine engine1;
      greedyController.connect( &engine1 );
      greedyController.subscribe( &engine1 );
      Execution::TimeWarp timeHandler1;
      timeHandler1.connect( &engine1 );

      Execution::CPController cpController( &flattenedGraph );
      Execution::CPSolutionObserver observer( &cpController.constraintProgramm );
      observer.subscribe( &engine1 );

      engine1.run(scenario.get());

      REQUIRE( observer.complete() );
      REQUIRE( observer.errors().empty() );

      auto scenario2 = dataProvider.createScenario();
      cpController.setSolution( CP::Solution(observer.getSolution()) );

      Execution::Engine engine2;
      cpController.connect( &engine2 );
      cpController.subscribe( &engine2 );
      Execution::TimeWarp timeHandler2;
      timeHandler2.connect( &engine2 );
      Execution::Recorder recorder;
      recorder.subscribe( &engine2 );

      engine2.run(scenario2.get());

      THEN( "The execution completes without errors" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );
      }
    }
  }
}
