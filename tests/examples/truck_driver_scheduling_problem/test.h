SCENARIO( "Truck driver scheduling problem", "[examples][truck_driver_scheduling_problem]" ) {
  const std::string modelFile = "examples/truck_driver_scheduling_problem/US_Truck_driver_scheduling_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two short trips without waiting" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Driver1; TruckDriverProcess; travel_times := [90,90]\n"
      "Driver1; TruckDriverProcess; service_times := [30,30]\n"
      "Driver1; TruckDriverProcess; earliest_visits := [0,0]\n"
      "Driver1; TruckDriverProcess; latest_visits := [1440,1440]\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      readyHandler.connect(&engine);

      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then no failure occurs" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then the process completes at the correct time" ) {
        auto processLog = recorder.find(nlohmann::json{{"state", "DONE"}},nlohmann::json{{"nodeId", nullptr}});
        REQUIRE( processLog.size() == 1 );
        REQUIRE( processLog.back()["status"]["timestamp"] == 240 );
//std::cout<< processLog.dump() << std::endl;
      }
    }
  }
  
  GIVEN( "One longer trip with waiting" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Driver1; TruckDriverProcess; travel_times := [500]\n"
      "Driver1; TruckDriverProcess; service_times := [30]\n"
      "Driver1; TruckDriverProcess; earliest_visits := [900]\n"
      "Driver1; TruckDriverProcess; latest_visits := [1440]\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      readyHandler.connect(&engine);

      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then no failure occurs" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then the process completes at the correct time" ) {
        auto processLog = recorder.find(nlohmann::json{{"state", "DONE"}},nlohmann::json{{"nodeId", nullptr}});
        REQUIRE( processLog.size() == 1 );
        REQUIRE( processLog.back()["status"]["timestamp"] == 930 );
      }
    }
  }

  GIVEN( "Multiple trips with alternative solutions" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Driver1; TruckDriverProcess; travel_times := [360,360]\n"  // Two 6h trips
      "Driver1; TruckDriverProcess; service_times := [30,30]\n"    // Two ½h services
      "Driver1; TruckDriverProcess; earliest_visits := [600,900]\n"
      "Driver1; TruckDriverProcess; latest_visits := [1800,1800]\n"
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

    WHEN( "The engine is started with the guided greedy controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      readyHandler.connect(&engine);

      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),2000);
      THEN( "Then no failure occurs" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
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

