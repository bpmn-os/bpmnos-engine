SCENARIO( "Pickup delivery problem", "[examples][pickup_delivery_problem]" ) {
  const std::string modelFile = "examples/pickup_delivery_problem/Pickup_delivery_problem.bpmn";
  BPMNOS::Model::LookupTable::folders = { "tests/examples/pickup_delivery_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A PDP with one vehicle and one customer" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;RequestsExpected;1\n"
      "VehicleProcess;Vehicle1;Depot;Hamburg\n"
      "VehicleProcess;Vehicle1;Capacity;1\n"
      "VehicleProcess;Vehicle1;EarliestAvailability;0\n"
      "VehicleProcess;Vehicle1;LatestAvailability;1440\n"
      "VehicleProcess;Vehicle1;OvertimePenalty;0\n"
      "CustomerProcess;Customer1;Quantity;1\n"
      "CustomerProcess;Customer1;PickupLocation;Berlin\n"
      "CustomerProcess;Customer1;PickupDuration;10\n"
      "CustomerProcess;Customer1;EarliestPickup;0\n"
      "CustomerProcess;Customer1;LatestPickup;1440\n"
      "CustomerProcess;Customer1;LatePickupPenalty;0\n"
      "CustomerProcess;Customer1;DeliveryLocation;Munich\n"
      "CustomerProcess;Customer1;DeliveryDuration;10\n"
      "CustomerProcess;Customer1;EarliestDelivery;0\n"
      "CustomerProcess;Customer1;LatestDelivery;1440\n"
      "CustomerProcess;Customer1;LateDeliveryPenalty;0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::ReadyHandler readyHandler;
    Execution::DeterministicTaskCompletion completionHandler;
    readyHandler.connect(&engine);
    completionHandler.connect(&engine);

    Execution::LocalEvaluator evaluator;
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
      THEN( "Then all vehicle process instances complete" ) {
        auto log = recorder.find({{"processId","VehicleProcess" },{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.dump() << std::endl;
        REQUIRE( log.size() == 1 );
      }
      THEN( "Then all customer process instances complete" ) {
        auto log = recorder.find({{"processId","CustomerProcess" },{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.dump() << std::endl;
        REQUIRE( log.size() == 1 );
      }
    }
  }

  GIVEN( "A PDP with one vehicle and two customers" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;RequestsExpected;2\n"
      "VehicleProcess;Vehicle1;Depot;Hamburg\n"
      "VehicleProcess;Vehicle1;Capacity;1\n"
      "VehicleProcess;Vehicle1;EarliestAvailability;0\n"
      "VehicleProcess;Vehicle1;LatestAvailability;1440\n"
      "VehicleProcess;Vehicle1;OvertimePenalty;0\n"
      "CustomerProcess;Customer1;Quantity;1\n"
      "CustomerProcess;Customer1;PickupLocation;Berlin\n"
      "CustomerProcess;Customer1;PickupDuration;10\n"
      "CustomerProcess;Customer1;EarliestPickup;0\n"
      "CustomerProcess;Customer1;LatestPickup;1440\n"
      "CustomerProcess;Customer1;LatePickupPenalty;0\n"
      "CustomerProcess;Customer1;DeliveryLocation;Munich\n"
      "CustomerProcess;Customer1;DeliveryDuration;10\n"
      "CustomerProcess;Customer1;EarliestDelivery;0\n"
      "CustomerProcess;Customer1;LatestDelivery;1440\n"
      "CustomerProcess;Customer1;LateDeliveryPenalty;0\n"
      "CustomerProcess;Customer2;Quantity;1\n"
      "CustomerProcess;Customer2;PickupLocation;Cologne\n"
      "CustomerProcess;Customer2;PickupDuration;10\n"
      "CustomerProcess;Customer2;EarliestPickup;0\n"
      "CustomerProcess;Customer2;LatestPickup;1440\n"
      "CustomerProcess;Customer2;LatePickupPenalty;0\n"
      "CustomerProcess;Customer2;DeliveryLocation;Hamburg\n"
      "CustomerProcess;Customer2;DeliveryDuration;10\n"
      "CustomerProcess;Customer2;EarliestDelivery;0\n"
      "CustomerProcess;Customer2;LatestDelivery;1440\n"
      "CustomerProcess;Customer2;LateDeliveryPenalty;0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::ReadyHandler readyHandler;
    Execution::DeterministicTaskCompletion completionHandler;
    readyHandler.connect(&engine);
    completionHandler.connect(&engine);

    Execution::LocalEvaluator evaluator;
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
      THEN( "Then all vehicle process instances complete" ) {
        auto log = recorder.find({{"processId","VehicleProcess" },{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.dump() << std::endl;
        REQUIRE( log.size() == 1 );
      }
      THEN( "Then all customer process instances complete" ) {
        auto log = recorder.find({{"processId","CustomerProcess" },{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.dump() << std::endl;
        REQUIRE( log.size() == 2 );
      }
    }
  }
}
