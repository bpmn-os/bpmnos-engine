SCENARIO( "Dynamic data provider", "[data][dynamic]" ) {
  const std::string modelFile = "tests/data/dynamic/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with instantiation after disclosure time" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 15; 5\n"
      "Instance_1; Process_1; x := 1; 10\n"
      "Instance_1; Activity_1; data := 5; 15\n"
      "Instance_1; Activity_1; y := 2; 20\n"
    ;

    WHEN( "The scenario is created" ) {
      Model::DynamicDataProvider dataProvider(modelFile, csv);
      auto scenario = dataProvider.createScenario();

      THEN( "Instance is not known before disclosure time of x" ) {
        auto instances = scenario->getKnownInstances(5);
        REQUIRE( instances.size() == 0 );
      }
      THEN( "Instance is known at disclosure time of x" ) {
        auto instances = scenario->getKnownInstances(10);
        REQUIRE( instances.size() == 1 );
      }
      THEN( "data and y are not known before its disclosure time" ) {
        auto instances = scenario->getKnownInstances(10);
        REQUIRE( instances.size() == 1 );
        auto instance = instances[0];
        auto& process = scenario->getModel()->processes[0];
        auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
        REQUIRE( activity != nullptr );
        
        auto data = scenario->getKnownData(instance->id, activity, 10);
        REQUIRE( !data.has_value() );
        auto status = scenario->getKnownValues(instance->id, activity, 10);
        REQUIRE( !status.has_value() );
      }
      THEN( "data is not disclosed before y" ) {
        auto instances = scenario->getKnownInstances(10);
        REQUIRE( instances.size() == 1 );
        auto instance = instances[0];
        auto& process = scenario->getModel()->processes[0];
        auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
        REQUIRE( activity != nullptr );

        auto data = scenario->getKnownData(instance->id, activity, 15);
        REQUIRE( !data.has_value() );
        auto status = scenario->getKnownValues(instance->id, activity, 15);
        REQUIRE( !status.has_value() );
      }
      THEN( "data and y are disclosed eventually" ) {
        auto instances = scenario->getKnownInstances(10);
        REQUIRE( instances.size() == 1 );
        auto instance = instances[0];
        auto& process = scenario->getModel()->processes[0];
        auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
        REQUIRE( activity != nullptr );

        auto data = scenario->getKnownData(instance->id, activity, 20);
        REQUIRE( data.has_value() );
        auto status = scenario->getKnownValues(instance->id, activity, 20);
        REQUIRE( status.has_value() );
      }
    }
  }

}
