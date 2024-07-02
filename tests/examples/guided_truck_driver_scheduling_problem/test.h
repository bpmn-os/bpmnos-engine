SCENARIO( "Truck driver scheduling problem", "[examples][truck_driver_scheduling_problem]" ) {
  const std::string modelFile = "examples/guided_truck_driver_scheduling_problem/Guided_US_Truck_driver_scheduling_problem.bpmn";
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

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      readyHandler.connect(&engine);
      completionHandler.connect(&engine);

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
      THEN( "Then no the process completes at the correct time" ) {
        auto processLog = recorder.find(nlohmann::json{{"state", "DONE"}},nlohmann::json{{"nodeId", nullptr}});
        REQUIRE( processLog.size() == 1 );
        REQUIRE( processLog.back()["status"]["timestamp"] == 240 );
//std::cout<< processLog.dump() << std::endl;
      }
    }
  }

  GIVEN( "Two one longer trip with waiting" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "TruckDriverProcess;Driver1;TravelTimes;[500]\n"
      "TruckDriverProcess;Driver1;ServiceTimes;[30]\n"
      "TruckDriverProcess;Driver1;EarliestVisits;[900]\n"
      "TruckDriverProcess;Driver1;LatestVisits;[1440]\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      readyHandler.connect(&engine);
      completionHandler.connect(&engine);

      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

//      Execution::Recorder recorder;
      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then no failure occurs" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then no the process completes at the correct time" ) {
        auto processLog = recorder.find(nlohmann::json{{"state", "DONE"}},nlohmann::json{{"nodeId", nullptr}});
        REQUIRE( processLog.size() == 1 );
        REQUIRE( processLog.back()["status"]["timestamp"] == 930 );
//std::cout<< processLog.dump() << std::endl;
      }
    }
  }

}

