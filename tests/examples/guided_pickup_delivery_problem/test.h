SCENARIO( "Guided pickup delivery problem", "[examples][pickup_delivery_problem]" ) {
  const std::string modelFile = "examples/guided_pickup_delivery_problem/Guided_pickup_delivery_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/guided_pickup_delivery_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "A FTL-PDP with one vehicle and one customer" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 1\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 1\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
      "Customer1; CustomerProcess; quantity := 1\n"
      "Customer1; CustomerProcess; pickup_location := \"Hamburg\"\n"
      "Customer1; CustomerProcess; pickup_duration := 10\n"
      "Customer1; CustomerProcess; earliest_pickup := 5\n"
      "Customer1; CustomerProcess; latest_pickup := 1440\n"
      "Customer1; CustomerProcess; late_pickup_penalty := 0\n"
      "Customer1; CustomerProcess; delivery_location := \"Hamburg\"\n"
      "Customer1; CustomerProcess; delivery_duration := 10\n"
      "Customer1; CustomerProcess; earliest_delivery := 30\n"
      "Customer1; CustomerProcess; latest_delivery := 1440\n"
      "Customer1; CustomerProcess; late_delivery_penalty := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,folders,csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::ReadyHandler readyHandler;
    Execution::DeterministicTaskCompletion completionHandler;
    readyHandler.connect(&engine);
    completionHandler.connect(&engine);

    Execution::GuidedEvaluator evaluator;
    Execution::GreedyController controller(&evaluator);
    controller.connect(&engine);
      
//    Execution::MyopicMessageTaskTerminator messageTaskTerminator;
//    messageTaskTerminator.connect(&engine);
    Execution::TimeWarp timeHandler;
    timeHandler.connect(&engine);

    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    WHEN( "The engine is started with the greedy controller" ) {
      engine.run(scenario.get());
      THEN( "Then both processes are completed" ) {
        auto processLog = recorder.find(nlohmann::json{{"state", "DONE"}},nlohmann::json{{"nodeId", nullptr},{"event", nullptr},{"decision", nullptr}});
        REQUIRE( processLog.size() == 1+1 );

//        auto decisionLog = recorder.find(nlohmann::json{{"decision", nullptr}});
//std::cerr << decisionLog.dump() << std::endl;
        REQUIRE( recorder.objective == -5 * 3 );

      }
    }
  }

  GIVEN( "A FTL-PDP with one vehicle and two customers" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 2\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 1\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
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
    ;

    Model::StaticDataProvider dataProvider(modelFile,folders,csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::ReadyHandler readyHandler;
    Execution::DeterministicTaskCompletion completionHandler;
    readyHandler.connect(&engine);
    completionHandler.connect(&engine);

    Execution::GuidedEvaluator evaluator;
    Execution::GreedyController controller(&evaluator);
    controller.connect(&engine);
      
//    Execution::MyopicMessageTaskTerminator messageTaskTerminator;
//    messageTaskTerminator.connect(&engine);
    Execution::TimeWarp timeHandler;
    timeHandler.connect(&engine);

    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    WHEN( "The engine is started with the greedy controller" ) {
      engine.run(scenario.get());
      THEN( "Then all processes are completed" ) {
        auto processLog = recorder.find(nlohmann::json{{"state", "DONE"}},nlohmann::json{{"nodeId", nullptr},{"event", nullptr},{"decision", nullptr}});
        REQUIRE( processLog.size() == 1+2 );

//        auto decisionLog = recorder.find(nlohmann::json{{"decision", nullptr}});
//std::cerr << decisionLog.dump() << std::endl;
      }
      THEN( "Then all pickup and delivery locations are visited" ) {
        // HH-B-M-K-HH-HH: 296 + 585 + 575 + 432 + 5= 1893
        REQUIRE( recorder.objective == -1893);
//        auto decisionLog = recorder.find(nlohmann::json{{"decision", nullptr}});
//std::cerr << decisionLog.dump() << std::endl;
      }
    }
  }
/*
  GIVEN( "A LTL-PDP with one vehicle and two customers" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 2\n"
      "Vehicle1; VehicleProcess; depot := Hamburg\n"
      "Vehicle1; VehicleProcess; capacity := 2\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
      "Customer1; CustomerProcess; quantity := 1\n"
      "Customer1; CustomerProcess; pickup_location := Berlin\n"
      "Customer1; CustomerProcess; pickup_duration := 10\n"
      "Customer1; CustomerProcess; earliest_pickup := 0\n"
      "Customer1; CustomerProcess; latest_pickup := 1440\n"
      "Customer1; CustomerProcess; late_pickup_penalty := 0\n"
      "Customer1; CustomerProcess; delivery_location := Munich\n"
      "Customer1; CustomerProcess; delivery_duration := 10\n"
      "Customer1; CustomerProcess; earliest_delivery := 0\n"
      "Customer1; CustomerProcess; latest_delivery := 1440\n"
      "Customer1; CustomerProcess; late_delivery_penalty := 0\n"
      "Customer2; CustomerProcess; quantity := 1\n"
      "Customer2; CustomerProcess; pickup_location := Cologne\n"
      "Customer2; CustomerProcess; pickup_duration := 10\n"
      "Customer2; CustomerProcess; earliest_pickup := 0\n"
      "Customer2; CustomerProcess; latest_pickup := 1440\n"
      "Customer2; CustomerProcess; late_pickup_penalty := 0\n"
      "Customer2; CustomerProcess; delivery_location := Hamburg\n"
      "Customer2; CustomerProcess; delivery_duration := 10\n"
      "Customer2; CustomerProcess; earliest_delivery := 0\n"
      "Customer2; CustomerProcess; latest_delivery := 1440\n"
      "Customer2; CustomerProcess; late_delivery_penalty := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,folders,csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::ReadyHandler readyHandler;
    Execution::DeterministicTaskCompletion completionHandler;
    readyHandler.connect(&engine);
    completionHandler.connect(&engine);

    Execution::GuidedEvaluator evaluator;
    Execution::GreedyController controller(&evaluator);
    controller.connect(&engine);
      
//    Execution::MyopicMessageTaskTerminator messageTaskTerminator;
//    messageTaskTerminator.connect(&engine);
    Execution::TimeWarp timeHandler;
    timeHandler.connect(&engine);

    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    WHEN( "The engine is started with the greedy controller" ) {
      engine.run(scenario.get());
      THEN( "Then all pickup and delivery locations are visited" ) {
        // HH-B-K-M-HH: 296 + 573 + 575 + 778 = 2222
        REQUIRE( recorder.objective == -2222 );
        
//        auto decisionLog = recorder.find(nlohmann::json{{"decision", nullptr}});
//std::cerr << decisionLog.dump() << std::endl;

      }
    }
  }
*/
}
