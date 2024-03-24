SCENARIO( "Lookup table", "[model][lookup]" ) {
  const std::string modelFile = "model/lookup/lookupTable.bpmn";
  BPMNOS::Model::LookupTable::folders = { std::string(std::filesystem::current_path()) + "/model/lookup" };
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance without existing client and server attributes" ) {
    WHEN( "The lookup is correct" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Client,Client2\n"
        "Process_1, Instance_1,Server,Server3\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      auto instantiations = scenario->getCurrentInstantiations(0);
      THEN( "The result is correct" ) {
        for ( auto& [process,values] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();
          REQUIRE( values.size() == 3 + 2 ); // don't forget timestamp and instance id
          REQUIRE( values[extensionElements->statusAttributes["cost"]->index].value() == 0 );
          extensionElements->applyOperators(values);
          REQUIRE( values[extensionElements->statusAttributes["cost"]->index].value() == 12 );
        }
      }
    }
  }

  GIVEN( "An instance without non-existing client and server attributes" ) {
    WHEN( "The lookup is correct" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Client,Client4\n"
        "Process_1, Instance_1,Server,Server3\n"
      ;
      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      auto instantiations = scenario->getCurrentInstantiations(0);
      THEN( "The result is correct" ) {
        for ( auto& [process,values] : instantiations ) {
          auto extensionElements = process->extensionElements->represents<Model::ExtensionElements>();
          REQUIRE( values.size() == 3 + 2 ); // don't forget timestamp and instance id
          REQUIRE( values[extensionElements->statusAttributes["cost"]->index].value() == 0 );
          extensionElements->applyOperators(values);
          REQUIRE( values[extensionElements->statusAttributes["cost"]->index].has_value() == false );
        }
      }
    }
  }
}


