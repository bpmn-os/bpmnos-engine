SCENARIO( "Truck driver scheduling problem", "[cpmodel][truck_driver_scheduling_problem]" ) {
  const std::string modelFile = "examples/truck_driver_scheduling_problem/US_Truck_driver_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two short trips without waiting" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "TruckDriverProcess;Driver1;TravelTimes;[90,90]\n"
      "TruckDriverProcess;Driver1;ServiceTimes;[30,30]\n"
      "TruckDriverProcess;Driver1;EarliestVisits;[0,0]\n"
      "TruckDriverProcess;Driver1;LatestVisits;[1440,1440]\n"
    ;
    WHEN( "The engine is started" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller( &flattenedGraph, &evaluator );

      Execution::Engine engine;
      controller.connect( &engine );
      controller.subscribe( &engine );
      Execution::TimeWarp timeHandler;
      timeHandler.connect( &engine );
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe( &engine );

      Execution::CPModel constraintProgramm( &flattenedGraph );
      Execution::CPSolutionObserver solution( &constraintProgramm );
      solution.subscribe( &engine );

      engine.run(scenario.get());

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

  GIVEN( "One longer trip with waiting" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "TruckDriverProcess;Driver1;TravelTimes;[500]\n"
      "TruckDriverProcess;Driver1;ServiceTimes;[30]\n"
      "TruckDriverProcess;Driver1;EarliestVisits;[900]\n"
      "TruckDriverProcess;Driver1;LatestVisits;[1440]\n"
    ;
    WHEN( "The engine is started" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller( &flattenedGraph, &evaluator );

      Execution::Engine engine;
      controller.connect( &engine );
      controller.subscribe( &engine );
      Execution::TimeWarp timeHandler;
      timeHandler.connect( &engine );
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe( &engine );

      Execution::CPModel constraintProgramm( &flattenedGraph );
      Execution::CPSolutionObserver solution( &constraintProgramm );
      solution.subscribe( &engine );

      engine.run(scenario.get());

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
}
