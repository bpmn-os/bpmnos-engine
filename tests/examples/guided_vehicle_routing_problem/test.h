SCENARIO( "Guided vehicle routing problem", "[examples][vehicle_routing_problem]" ) {
  const std::string modelFile = "examples/guided_vehicle_routing_problem/Guided_vehicle_routing_problem.bpmn";
  BPMNOS::Model::LookupTable::folders = { "tests/examples/guided_vehicle_routing_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A VRP with one vehicle and two customers" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;RequestsExpected;2\n"
      "VehicleProcess;Vehicle1;Depot;\"Hamburg\"\n"
      "VehicleProcess;Vehicle1;Capacity;2\n"
      "VehicleProcess;Vehicle1;EarliestAvailability;0\n"
      "VehicleProcess;Vehicle1;LatestAvailability;1440\n"
      "VehicleProcess;Vehicle1;OvertimePenalty;0\n"
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
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
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
      engine.run(scenario.get(),1350);
      THEN( "Then all pickup and delivery locations are visited" ) {
        // HH-B-K-HH: 296 + 573 + 432 = 1301
        REQUIRE( recorder.objective == -1301 );
/*        
        auto decisionLog = recorder.find(nlohmann::json{{"decision", nullptr}});
std::cerr << decisionLog.dump() << std::endl;
*/
      }
    }
  }
}
