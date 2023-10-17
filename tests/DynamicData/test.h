SCENARIO( "Trivial executable process", "[data][dynamic]" ) {
  const std::string modelFile = "DynamicData/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with dynamically disclosed timestamp" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE, DISCLOSURE\n"
      "Process_1, Instance_1,Timestamp,35,15\n"
      "Process_1, Instance_1,Timestamp,42,35\n"
    ;

    WHEN( "The instance is loaded" ) {
      Model::DynamicDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      THEN( "There is no instance created by time 0" ) {
        auto instances = scenario->getCreatedInstances(0);
        REQUIRE( instances.size() == 0 );
      }
      THEN( "There is no instance created by time 35" ) {
        auto instances = scenario->getCreatedInstances(35);
        REQUIRE( instances.size() == 0 );
      }
      THEN( "Exactly one instantiation is created by time 42" ) {
        auto instances = scenario->getCreatedInstances(42);
        REQUIRE( instances.size() == 1 );
      }
      THEN( "No instantiation is known to occur at time 35" ) {
        auto instantiations = scenario->getInstantiations(35);
        REQUIRE( instantiations.size() == 0 );
      }
      THEN( "Exactly one instantiation is known to occur at time 42" ) {
        auto instantiations = scenario->getInstantiations(42);
        REQUIRE( instantiations.size() == 1 );
      }
      THEN( "There is no instance known by time 0" ) {
        auto instances = scenario->getKnownInstances(0);
        REQUIRE( instances.size() == 0 );
      }
      THEN( "There is exactly one instance known by time 35" ) {
        auto instances = scenario->getKnownInstances(35);
        REQUIRE( instances.size() == 1 );
      }
      THEN( "There is no instance anticipated at time 0" ) {
        auto instances = scenario->getAnticipatedInstances(0);
        REQUIRE( instances.size() == 0 );
      }
      THEN( "There is exactly one instance anticipated at time 15" ) {
        auto instances = scenario->getAnticipatedInstances(15);
        REQUIRE( instances.size() == 1 );
      }
      THEN( "There is no unknown instance anticipated at time 35" ) {
        auto instances = scenario->getAnticipatedInstances(35);
        REQUIRE( instances.size() == 0 );
      }
      THEN( "At time 42 the instantiation data is known" ) {
        std::string instanceId = "Instance_1";
        BPMNOS::number timestamp = 42;
        auto instantiations = scenario->getInstantiations(timestamp);
        auto& [process,values] = instantiations.front();
        REQUIRE( values.size() == 2 );
        REQUIRE( values[Model::Status::Index::Instance].value() == BPMNOS::to_number(instanceId,STRING) );
        REQUIRE( values[Model::Status::Index::Timestamp].value() == timestamp );
      }
    }
  }
}
