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
      THEN( "The instance data is correct" ) {
        for ( auto& instance : instances ) {
          REQUIRE( instance->process->id == "Process_1" );
          REQUIRE( instance->id == "Instance_1" );
          auto status = instance->process->extensionElements->represents<Model::Status>();
          REQUIRE( status->attributes.size() == 2 );
          REQUIRE( (std::string)status->attributes[0]->id == Keyword::Instance );
          REQUIRE( status->attributes[0]->isImmutable == true );
          REQUIRE( status->attributes[0]->value == std::nullopt );
          REQUIRE( (std::string)status->attributes[1]->id == Keyword::Timestamp );
          REQUIRE( status->attributes[1]->isImmutable == false );
          REQUIRE( status->attributes[1]->value.value() == 0 );
        }
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
      THEN( "The instance data is correct" ) {
        for ( auto& instance : instances ) {
          REQUIRE( instance->process->id == "Process_1" );
          REQUIRE( instance->id == "Instance_1" );

          auto status = instance->process->extensionElements->represents<Model::Status>();
          REQUIRE( status->attributes.size() == 2 );
          REQUIRE( (std::string)status->attributes[0]->id == Keyword::Instance );
          REQUIRE( status->attributes[0]->isImmutable == true );
          REQUIRE( status->attributes[0]->value == std::nullopt );
          REQUIRE( (std::string)status->attributes[1]->id == Keyword::Timestamp );
          REQUIRE( status->attributes[1]->isImmutable == false );
          REQUIRE( status->attributes[1]->value.value() == 0 );
        }
      }
    }
  }
}
