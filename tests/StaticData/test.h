SCENARIO( "Trivial executable process", "[data][static]" ) {
  const std::string modelFile = "StaticData/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n";

    WHEN( "The instance is loaded" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instances = scenario->getKnownInstances(0);

      THEN( "There is exactly one instance" ) {
        REQUIRE( instances.size() == 1 );
      }
      THEN( "The model data is correct" ) {
        for ( auto& instance : instances ) {
          REQUIRE( instance->process->id == "Process_1" );
          REQUIRE( instance->id == "Instance_1" );
          auto status = instance->process->extensionElements->represents<Model::Status>();
          REQUIRE( status->attributes.size() == 2 );
          REQUIRE( (std::string)status->attributes[Model::Status::Index::Instance]->id == Keyword::Instance );
          REQUIRE( status->attributes[Model::Status::Index::Instance]->isImmutable == true );
          REQUIRE( status->attributes[Model::Status::Index::Instance]->value == std::nullopt );
          REQUIRE( (std::string)status->attributes[Model::Status::Index::Timestamp]->id == Keyword::Timestamp );
          REQUIRE( status->attributes[Model::Status::Index::Timestamp]->isImmutable == false );
          REQUIRE( status->attributes[Model::Status::Index::Timestamp]->value.value() == 0 );
        }
      }
      THEN( "The instantiation data is correct" ) {
        std::string instanceId = "Instance_1";
        BPMNOS::number timestamp = 0;
        auto instantiations = scenario->getKnownInstantiations(0);
        REQUIRE( instantiations.size() == 1 );
        auto& [process,values] = instantiations.front();
        REQUIRE( values.size() == 2 );
        REQUIRE( values[Model::Status::Index::Instance].value() == BPMNOS::to_number(instanceId,STRING) );
        REQUIRE( values[Model::Status::Index::Timestamp].value() == timestamp );
      }
    }
  }
  GIVEN( "A single instance with instance and timestamp input" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Instance,Instance_1\n"
      "Process_1, Instance_1,Timestamp,42\n";

    WHEN( "The instance is loaded" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instances = scenario->getKnownInstances(0);

      THEN( "There is exactly one instance" ) {
        REQUIRE( instances.size() == 1 );
      }
      THEN( "The model data is correct" ) {
        for ( auto& instance : instances ) {
          REQUIRE( instance->process->id == "Process_1" );
          REQUIRE( instance->id == "Instance_1" );

          auto status = instance->process->extensionElements->represents<Model::Status>();
          REQUIRE( status->attributes.size() == 2 );
          REQUIRE( (std::string)status->attributes[Model::Status::Index::Instance]->id == Keyword::Instance );
          REQUIRE( status->attributes[Model::Status::Index::Instance]->isImmutable == true );
          REQUIRE( status->attributes[Model::Status::Index::Instance]->value == std::nullopt );
          REQUIRE( (std::string)status->attributes[Model::Status::Index::Timestamp]->id == Keyword::Timestamp );
          REQUIRE( status->attributes[Model::Status::Index::Timestamp]->isImmutable == false );
          REQUIRE( status->attributes[Model::Status::Index::Timestamp]->value.value() == 0 );
        }
      }
      THEN( "Exactly one instantiation is known to occur at time 42" ) {
        auto instantiations = scenario->getKnownInstantiations(42);
        REQUIRE( instantiations.size() == 1 );
      }
      THEN( "No instantiation is known to occur at time 0" ) {
        auto instantiations = scenario->getKnownInstantiations(0);
        REQUIRE( instantiations.size() == 0 );
      }
      THEN( "Exactly one instantiation is assumed to occur at time 42" ) {
        auto instantiations = scenario->getAssumedInstantiations(0,42);
        REQUIRE( instantiations.size() == 1 );
      }
      THEN( "No instantiation is assumed to occur at time 0" ) {
        auto instantiations = scenario->getAssumedInstantiations(0,0);
        REQUIRE( instantiations.size() == 0 );
      }
      THEN( "No instantiation is assumed to occur after time 42" ) {
        auto instantiations = scenario->getAssumedInstantiations(0,43);
        REQUIRE( instantiations.size() == 0 );
      }
      THEN( "The instantiation data is correct" ) {
        std::string instanceId = "Instance_1";
        BPMNOS::number timestamp = 42;
        auto instantiations = scenario->getKnownInstantiations(timestamp);
        auto& [process,values] = instantiations.front();
        REQUIRE( values.size() == 2 );
        REQUIRE( values[Model::Status::Index::Instance].value() == BPMNOS::to_number(instanceId,STRING) );
        REQUIRE( values[Model::Status::Index::Timestamp].value() == timestamp );
      }
    }
  }
}
