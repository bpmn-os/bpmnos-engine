SCENARIO( "CPController with pickup delivery problem", "[cpcontroller][pickup_delivery_problem]" ) {
  const std::string modelFile = "examples/pickup_delivery_problem/Pickup_delivery_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/pickup_delivery_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "A PDP with one vehicle and no customers" ) {
    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;RequestsExpected;0\n"
      "VehicleProcess;Vehicle1;Depot;\"Hamburg\"\n"
      "VehicleProcess;Vehicle1;Capacity;1\n"
      "VehicleProcess;Vehicle1;EarliestAvailability;0\n"
      "VehicleProcess;Vehicle1;LatestAvailability;1440\n"
      "VehicleProcess;Vehicle1;OvertimePenalty;0\n"
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

  GIVEN( "A PDP with one vehicle and one customer" ) {
    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;RequestsExpected;1\n"
      "CustomerProcess;Customer1;Quantity;1\n"
      "CustomerProcess;Customer1;PickupLocation;\"Berlin\"\n"
      "CustomerProcess;Customer1;PickupDuration;10\n"
      "CustomerProcess;Customer1;EarliestPickup;0\n"
      "CustomerProcess;Customer1;LatestPickup;1440\n"
      "CustomerProcess;Customer1;LatePickupPenalty;0\n"
      "CustomerProcess;Customer1;DeliveryLocation;\"Munich\"\n"
      "CustomerProcess;Customer1;DeliveryDuration;10\n"
      "CustomerProcess;Customer1;EarliestDelivery;0\n"
      "CustomerProcess;Customer1;LatestDelivery;1440\n"
      "CustomerProcess;Customer1;LateDeliveryPenalty;0\n"
      "VehicleProcess;Vehicle1;Depot;\"Hamburg\"\n"
      "VehicleProcess;Vehicle1;Capacity;1\n"
      "VehicleProcess;Vehicle1;EarliestAvailability;0\n"
      "VehicleProcess;Vehicle1;LatestAvailability;1440\n"
      "VehicleProcess;Vehicle1;OvertimePenalty;0\n"
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

  GIVEN( "A PDP with one vehicle and two customers" ) {
    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;RequestsExpected;2\n"
      "CustomerProcess;Customer1;Quantity;1\n"
      "CustomerProcess;Customer1;PickupLocation;\"Berlin\"\n"
      "CustomerProcess;Customer1;PickupDuration;10\n"
      "CustomerProcess;Customer1;EarliestPickup;0\n"
      "CustomerProcess;Customer1;LatestPickup;1440\n"
      "CustomerProcess;Customer1;LatePickupPenalty;0\n"
      "CustomerProcess;Customer1;DeliveryLocation;\"Munich\"\n"
      "CustomerProcess;Customer1;DeliveryDuration;10\n"
      "CustomerProcess;Customer1;EarliestDelivery;0\n"
      "CustomerProcess;Customer1;LatestDelivery;1440\n"
      "CustomerProcess;Customer1;LateDeliveryPenalty;0\n"
      "CustomerProcess;Customer2;Quantity;1\n"
      "CustomerProcess;Customer2;PickupLocation;\"Cologne\"\n"
      "CustomerProcess;Customer2;PickupDuration;10\n"
      "CustomerProcess;Customer2;EarliestPickup;0\n"
      "CustomerProcess;Customer2;LatestPickup;1440\n"
      "CustomerProcess;Customer2;LatePickupPenalty;0\n"
      "CustomerProcess;Customer2;DeliveryLocation;\"Hamburg\"\n"
      "CustomerProcess;Customer2;DeliveryDuration;10\n"
      "CustomerProcess;Customer2;EarliestDelivery;0\n"
      "CustomerProcess;Customer2;LatestDelivery;1440\n"
      "CustomerProcess;Customer2;LateDeliveryPenalty;0\n"
      "VehicleProcess;Vehicle1;Depot;\"Hamburg\"\n"
      "VehicleProcess;Vehicle1;Capacity;1\n"
      "VehicleProcess;Vehicle1;EarliestAvailability;0\n"
      "VehicleProcess;Vehicle1;LatestAvailability;1440\n"
      "VehicleProcess;Vehicle1;OvertimePenalty;0\n"
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
