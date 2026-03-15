SCENARIO( "Guided vehicle routing problem", "[examples][vehicle_routing_problem]" ) {
  const std::string modelFile = "examples/guided_vehicle_routing_problem/Guided_vehicle_routing_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/guided_vehicle_routing_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "A VRP with one vehicle and two customers" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; requests_expected := 2\n"
      "Vehicle1; VehicleProcess; depot := \"Hamburg\"\n"
      "Vehicle1; VehicleProcess; capacity := 2\n"
      "Vehicle1; VehicleProcess; earliest_availability := 0\n"
      "Vehicle1; VehicleProcess; latest_availability := 1440\n"
      "Vehicle1; VehicleProcess; overtime_penalty := 0\n"
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
    ;

    Model::StaticDataProvider dataProvider(modelFile,folders,csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;

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
