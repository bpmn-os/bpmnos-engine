SCENARIO( "CPController with truck driver scheduling problem", "[cpcontroller][truck_driver_scheduling_problem]" ) {
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

    Model::StaticDataProvider dataProvider(modelFile,csv);
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

  GIVEN( "One longer trip with waiting" ) {
    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "TruckDriverProcess;Driver1;TravelTimes;[500]\n"
      "TruckDriverProcess;Driver1;ServiceTimes;[30]\n"
      "TruckDriverProcess;Driver1;EarliestVisits;[900]\n"
      "TruckDriverProcess;Driver1;LatestVisits;[1440]\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
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

  GIVEN( "Multiple trips with alternative solutions" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "TruckDriverProcess;Driver1;TravelTimes;[360,360]\n"  // Two 6h trips
      "TruckDriverProcess;Driver1;ServiceTimes;[30,30]\n"    // Two ½h services
      "TruckDriverProcess;Driver1;EarliestVisits;[600,900]\n"
      "TruckDriverProcess;Driver1;LatestVisits;[1800,1800]\n"
    ;
    // T_MAX_DRIVING_SINCE_REST: 11h
    // T_MAX_ELAPSED_SINCE_REST: 14h
    // T_MAX_ELAPSED_SINCE_BREAK: 8h
    // T_REST: 10h
    // T_BREAK: ½h
    // Solution 1a: D6, B½,W3½, S½, D3, R10, D3, S½ => 27h
    // Solution 1b: D6, W4, S½, D2, B½, D1, R10, D3, S½ => 27½h
    // Solution 2: D6, R10, S½, D6, S½ => 23h

    Model::StaticDataProvider dataProvider(modelFile,csv);
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
      THEN( "Then no the process completes at the correct non-optimal time" ) {
        auto processLog = recorder.find(nlohmann::json{{"state", "DONE"}},nlohmann::json{{"nodeId", nullptr}});
        REQUIRE( processLog.size() == 1 );
        // Solution 1a or 1b
        REQUIRE( (processLog.back()["status"]["timestamp"] == 1620 || processLog.back()["status"]["timestamp"] == 1650) ); 
      }
    }
  }

}
