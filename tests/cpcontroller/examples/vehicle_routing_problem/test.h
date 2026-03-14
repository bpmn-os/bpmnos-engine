SCENARIO( "CPController with vehicle routing problem", "[cpcontroller][vehicle_routing_problem]" ) {
  const std::string modelFile = "examples/vehicle_routing_problem/Vehicle_routing_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/vehicle_routing_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "A VRP with one vehicle and no customers" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 0\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 2\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
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

  GIVEN( "A VRP with one vehicle and one customer" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 1\n"
      "Customer1; CustomerProcess; quantity := 1\n"
      "Customer1; CustomerProcess; location := \"Berlin\"\n"
      "Customer1; CustomerProcess; handling_duration := 10\n"
      "Customer1; CustomerProcess; earliest_visit := 0\n"
      "Customer1; CustomerProcess; latest_visit := 1440\n"
      "Customer1; CustomerProcess; lateness_penalty := 0\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 2\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
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

  GIVEN( "A VRP with one vehicle and two customers" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 2\n"
      "Customer1; CustomerProcess; quantity := 1\n"
      "Customer1; CustomerProcess; location := \"Berlin\"\n"
      "Customer1; CustomerProcess; handling_duration := 10\n"
      "Customer1; CustomerProcess; earliest_visit := 0\n"
      "Customer1; CustomerProcess; latest_visit := 1440\n"
      "Customer1; CustomerProcess; lateness_penalty := 0\n"
      "Customer2; CustomerProcess; quantity := 1\n"
      "Customer2; CustomerProcess; location := \"Cologne\"\n"
      "Customer2; CustomerProcess; handling_duration := 10\n"
      "Customer2; CustomerProcess; earliest_visit := 0\n"
      "Customer2; CustomerProcess; latest_visit := 1440\n"
      "Customer2; CustomerProcess; lateness_penalty := 0\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 2\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
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
