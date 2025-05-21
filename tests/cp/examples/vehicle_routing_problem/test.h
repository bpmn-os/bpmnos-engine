SCENARIO( "Vehicle routing problem", "[cp][vehicle_routing_problem]" ) {
  const std::string modelFile = "examples/vehicle_routing_problem/Vehicle_routing_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/vehicle_routing_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "A VRP with one vehicle and no customers" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;RequestsExpected;0\n"
      "VehicleProcess;Vehicle1;Depot;\"Hamburg\"\n"
      "VehicleProcess;Vehicle1;Capacity;2\n"
      "VehicleProcess;Vehicle1;EarliestAvailability;0\n"
      "VehicleProcess;Vehicle1;LatestAvailability;1440\n"
      "VehicleProcess;Vehicle1;OvertimePenalty;0\n"
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
      Execution::CPSolution solution( &constraintProgramm );
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

  GIVEN( "A VRP with one vehicle and one customer" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;RequestsExpected;1\n"
      "CustomerProcess;Customer1;Quantity;1\n"
      "CustomerProcess;Customer1;Location;\"Berlin\"\n"
      "CustomerProcess;Customer1;HandlingDuration;10\n"
      "CustomerProcess;Customer1;EarliestVisit;0\n"
      "CustomerProcess;Customer1;LatestVisit;1440\n"
      "CustomerProcess;Customer1;LatenessPenalty;0\n"
      "VehicleProcess;Vehicle1;Depot;\"Hamburg\"\n"
      "VehicleProcess;Vehicle1;Capacity;2\n"
      "VehicleProcess;Vehicle1;EarliestAvailability;0\n"
      "VehicleProcess;Vehicle1;LatestAvailability;1440\n"
      "VehicleProcess;Vehicle1;OvertimePenalty;0\n"
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
      Execution::CPSolution solution( &constraintProgramm );
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

  GIVEN( "A VRP with one vehicle and two customers" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;RequestsExpected;2\n"
      "CustomerProcess;Customer1;Quantity;1\n"
      "CustomerProcess;Customer1;Location;\"Berlin\"\n"
      "CustomerProcess;Customer1;HandlingDuration;10\n"
      "CustomerProcess;Customer1;EarliestVisit;0\n"
      "CustomerProcess;Customer1;LatestVisit;1440\n"
      "CustomerProcess;Customer1;LatenessPenalty;0\n"
      "CustomerProcess;Customer2;Quantity;1\n"
      "CustomerProcess;Customer2;Location;\"Cologne\"\n"
      "CustomerProcess;Customer2;HandlingDuration;10\n"
      "CustomerProcess;Customer2;EarliestVisit;0\n"
      "CustomerProcess;Customer2;LatestVisit;1440\n"
      "CustomerProcess;Customer2;LatenessPenalty;0\n"
      "VehicleProcess;Vehicle1;Depot;\"Hamburg\"\n"
      "VehicleProcess;Vehicle1;Capacity;2\n"
      "VehicleProcess;Vehicle1;EarliestAvailability;0\n"
      "VehicleProcess;Vehicle1;LatestAvailability;1440\n"
      "VehicleProcess;Vehicle1;OvertimePenalty;0\n"
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
      Execution::CPSolution solution( &constraintProgramm );
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

