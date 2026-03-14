SCENARIO( "Pickup delivery problem", "[cpmodel][pickup_delivery_problem]" ) {
  const std::string modelFile = "examples/pickup_delivery_problem/Pickup_delivery_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/pickup_delivery_problem" };

  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "A PDP with one vehicle and no customers" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 0\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 1\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
    ;

    WHEN( "The engine is started" ) {
      Model::StaticDataProvider dataProvider(modelFile,folders,csv);
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
      
//std::cerr << "Model:\n" << constraintProgramm.stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors: " << solution.errors() << std::endl;

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

  GIVEN( "A PDP with one vehicle and one customer" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 1\n"
      "Customer1; CustomerProcess; quantity := 1\n"
      "Customer1; CustomerProcess; pickup_location := \"Berlin\"\n"
      "Customer1; CustomerProcess; pickup_duration := 10\n"
      "Customer1; CustomerProcess; earliest_pickup := 0\n"
      "Customer1; CustomerProcess; latest_pickup := 1440\n"
      "Customer1; CustomerProcess; late_pickup_penalty := 0\n"
      "Customer1; CustomerProcess; delivery_location := \"Munich\"\n"
      "Customer1; CustomerProcess; delivery_duration := 10\n"
      "Customer1; CustomerProcess; earliest_delivery := 0\n"
      "Customer1; CustomerProcess; latest_delivery := 1440\n"
      "Customer1; CustomerProcess; late_delivery_penalty := 0\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 1\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
    ;

    WHEN( "The engine is started" ) {
      Model::StaticDataProvider dataProvider(modelFile,folders,csv);
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
      
//std::cerr << "Model:\n" << constraintProgramm.stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors: " << solution.errors() << std::endl;

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

  GIVEN( "A PDP with one vehicle and two customers" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 2\n"
      "Customer1; CustomerProcess; quantity := 1\n"
      "Customer1; CustomerProcess; pickup_location := \"Berlin\"\n"
      "Customer1; CustomerProcess; pickup_duration := 10\n"
      "Customer1; CustomerProcess; earliest_pickup := 0\n"
      "Customer1; CustomerProcess; latest_pickup := 1440\n"
      "Customer1; CustomerProcess; late_pickup_penalty := 0\n"
      "Customer1; CustomerProcess; delivery_location := \"Munich\"\n"
      "Customer1; CustomerProcess; delivery_duration := 10\n"
      "Customer1; CustomerProcess; earliest_delivery := 0\n"
      "Customer1; CustomerProcess; latest_delivery := 1440\n"
      "Customer1; CustomerProcess; late_delivery_penalty := 0\n"
      "Customer2; CustomerProcess; quantity := 1\n"
      "Customer2; CustomerProcess; pickup_location := \"Cologne\"\n"
      "Customer2; CustomerProcess; pickup_duration := 10\n"
      "Customer2; CustomerProcess; earliest_pickup := 0\n"
      "Customer2; CustomerProcess; latest_pickup := 1440\n"
      "Customer2; CustomerProcess; late_pickup_penalty := 0\n"
      "Customer2; CustomerProcess; delivery_location := \"Hamburg\"\n"
      "Customer2; CustomerProcess; delivery_duration := 10\n"
      "Customer2; CustomerProcess; earliest_delivery := 0\n"
      "Customer2; CustomerProcess; latest_delivery := 1440\n"
      "Customer2; CustomerProcess; late_delivery_penalty := 0\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 1\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
    ;

    WHEN( "The engine is started" ) {
      Model::StaticDataProvider dataProvider(modelFile,folders,csv);
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
      
//std::cerr << "Model:\n" << constraintProgramm.stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors: " << solution.errors() << std::endl;

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

