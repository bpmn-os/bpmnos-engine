SCENARIO( "Trivial executable process", "[data][static]" ) {
  const std::string modelFile = "StaticData/Executable_process.bpmn";
  BPMNOS::Model model(modelFile);

  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n";

    WHEN( "The instance is loaded" ) {
      BPMNOS::StaticDataProvider dataProvider(modelFile,csv);
      auto& instances = dataProvider.getInstances();
      THEN( "There is exactly one instance" ) {
        REQUIRE( instances.size() == 1 );
      }
      THEN( "The instance data is correct" ) {
        for ( auto& [id,instance] : instances ) {
          REQUIRE( instance->process->id == "Process_1" );
          REQUIRE( instance->id == "Instance_1" );

          auto status = instance->process->extensionElements->represents<Status>();
          REQUIRE( status->attributes.size() == 2 );
          REQUIRE( (std::string)status->attributes[0]->name == "instance" );
          REQUIRE( status->attributes[0]->value == std::nullopt );
          REQUIRE( (std::string)status->attributes[1]->name == "timestamp" );
          REQUIRE( std::get<int>(status->attributes[1]->value.value()) == 0 );

          std::vector<std::optional<int> > values;
          dataProvider.appendActualValues<int>(instance.get(),instance->process,values);
          REQUIRE( values.size() == 2 );
          REQUIRE( values[0].value() == stringRegistry("Instance_1") );
          REQUIRE( values[1].value() == 0 );
        }
      }
    }
  }
  GIVEN( "A single instance with instance and timestamp input" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Attribute_37jt054,Instance_1\n"
      "Process_1, Instance_1,Attribute_0utbkub,42\n";

    WHEN( "The instance is loaded" ) {
      BPMNOS::StaticDataProvider dataProvider(modelFile,csv);
      auto& instances = dataProvider.getInstances();
      THEN( "There is exactly one instance" ) {
        REQUIRE( instances.size() == 1 );
      }
      THEN( "The instance data is correct" ) {
        for ( auto& [id,instance] : instances ) {
          REQUIRE( instance->process->id == "Process_1" );
          REQUIRE( instance->id == "Instance_1" );

          auto status = instance->process->extensionElements->represents<Status>();
          REQUIRE( status->attributes.size() == 2 );
          REQUIRE( (std::string)status->attributes[0]->name == "instance" );
          REQUIRE( status->attributes[0]->value == std::nullopt );
          REQUIRE( (std::string)status->attributes[1]->name == "timestamp" );
          REQUIRE( std::get<int>(status->attributes[1]->value.value()) == 0 );

          std::vector<std::optional<double> > values;
          dataProvider.appendActualValues<double>(instance.get(),instance->process,values);
          REQUIRE( values.size() == 2 );
          REQUIRE( values[0].value() == stringRegistry("Instance_1") );
          REQUIRE( values[1].value() == 42 );
        }
      }
    }
  }
}
