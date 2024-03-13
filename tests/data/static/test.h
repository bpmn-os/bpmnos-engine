SCENARIO( "Trivial executable process", "[data][static]" ) {
  const std::string modelFile = "data/static/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n";

    WHEN( "The instance is loaded" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      auto instances = scenario->getCreatedInstances(0);

      THEN( "There is exactly one instance" ) {
        REQUIRE( instances.size() == 1 );
      }
      THEN( "The model data is correct" ) {
        for ( auto& instance : instances ) {
          REQUIRE( instance->process->id == "Process_1" );
          REQUIRE( instance->id == "Instance_1" );
          auto extensionElements = instance->process->extensionElements->represents<Model::ExtensionElements>();
          REQUIRE( extensionElements->attributes.size() == 2 );
          REQUIRE( (std::string)extensionElements->attributes[Model::ExtensionElements::Index::Instance]->id == Keyword::Instance );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Instance]->isImmutable == true );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Instance]->value == std::nullopt );
          REQUIRE( (std::string)extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->id == Keyword::Timestamp );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->isImmutable == false );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->value.value() == 0 );
        }
      }
      THEN( "The instantiation data is correct" ) {
        std::string instanceId = "Instance_1";
        BPMNOS::number timestamp = 0;
        auto instantiations = scenario->getCurrentInstantiations(0);
        REQUIRE( instantiations.size() == 1 );
        auto& [process,values] = instantiations.front();
        REQUIRE( values.size() == 2 );
        REQUIRE( values[Model::ExtensionElements::Index::Instance].value() == BPMNOS::to_number(instanceId,STRING) );
        REQUIRE( values[Model::ExtensionElements::Index::Timestamp].value() == timestamp );
      }
    }
  }
  GIVEN( "A single instance with instance and timestamp input" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Instance,Instance_1\n"
      "Process_1, Instance_1,Timestamp,42\n"
    ;

    WHEN( "The instance is loaded" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      THEN( "There earliest instantiation is at time 42" ) {
        REQUIRE( scenario->getInception() == 42 );
      }
      THEN( "The scenario is incomplete at time 42" ) {
        REQUIRE( scenario->isCompleted(42) == false );
      }
      THEN( "The scenario is complete at time 43" ) {
        REQUIRE( scenario->isCompleted(43) == true );
      }
      THEN( "There is mo instance created by time 0" ) {
        auto anticipatedInstances = scenario->getCreatedInstances(0);
        REQUIRE( anticipatedInstances.size() == 0 );
      }
      THEN( "There is exactly one instance created by time 42" ) {
        auto anticipatedInstances = scenario->getCreatedInstances(42);
        REQUIRE( anticipatedInstances.size() == 1 );
      }
      THEN( "There is exactly one instance known at time 42" ) {
        auto anticipatedInstances = scenario->getKnownInstances(42);
        REQUIRE( anticipatedInstances.size() == 1 );
      }
      THEN( "Exactly one instantiation is known to occur at time 42" ) {
        auto instantiations = scenario->getCurrentInstantiations(42);
        REQUIRE( instantiations.size() == 1 );
      }
      THEN( "No instantiation is known to occur at time 0" ) {
        auto instantiations = scenario->getCurrentInstantiations(0);
        REQUIRE( instantiations.size() == 0 );
      }
      THEN( "The model data is correct" ) {
        auto instances = scenario->getKnownInstances(0);
        for ( auto& instance : instances ) {
          auto extensionElements = instance->process->extensionElements->represents<Model::ExtensionElements>();
          REQUIRE( extensionElements->attributes.size() == 2 );
          REQUIRE( (std::string)extensionElements->attributes[Model::ExtensionElements::Index::Instance]->id == Keyword::Instance );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Instance]->isImmutable == true );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Instance]->value == std::nullopt );
          REQUIRE( (std::string)extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->id == Keyword::Timestamp );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->isImmutable == false );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->value.value() == 0 );
        }
      }
      THEN( "The instantiation data of the first instance is correct" ) {
        std::string instanceId = "Instance_1";
        BPMNOS::number timestamp = 42;
        auto instantiations = scenario->getCurrentInstantiations(timestamp);
        auto& [process,values] = instantiations.front();
        REQUIRE( values.size() == 2 );
        REQUIRE( values[Model::ExtensionElements::Index::Instance].value() == BPMNOS::to_number(instanceId,STRING) );
        REQUIRE( values[Model::ExtensionElements::Index::Timestamp].value() == timestamp );
      }
    }
  }
  GIVEN( "Two instances, one with instance and timestamp input" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Instance,Instance_1\n"
      "Process_1, Instance_1,Timestamp,42\n"
      "Process_1, Instance_2,,\n"
    ;

    WHEN( "The instance is loaded" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      THEN( "There earliest instantiation is at time 0" ) {
        REQUIRE( scenario->getInception() == 0 );
      }
      THEN( "The scenario is incomplete at time 0" ) {
        REQUIRE( scenario->isCompleted(0) == false );
      }
      THEN( "The scenario is incomplete at time 42" ) {
        REQUIRE( scenario->isCompleted(42) == false );
      }
      THEN( "The scenario is complete at time 43" ) {
        REQUIRE( scenario->isCompleted(43) == true );
      }
      THEN( "There is exactly one instance created by time 0" ) {
        auto anticipatedInstances = scenario->getCreatedInstances(0);
        REQUIRE( anticipatedInstances.size() == 1 );
      }
      THEN( "There are exactly two instances created by time 42" ) {
        auto anticipatedInstances = scenario->getCreatedInstances(43);
        REQUIRE( anticipatedInstances.size() == 2 );
      }
      THEN( "There are exactly two instances known at time 0" ) {
        auto anticipatedInstances = scenario->getKnownInstances(0);
        REQUIRE( anticipatedInstances.size() == 2 );
      }
      THEN( "Exactly one instantiation is known to occur at time 42" ) {
        auto instantiations = scenario->getCurrentInstantiations(42);
        REQUIRE( instantiations.size() == 1 );
      }
      THEN( "Exactly one instantiation is known to occur at time 0" ) {
        auto instantiations = scenario->getCurrentInstantiations(0);
        REQUIRE( instantiations.size() == 1 );
      }
      THEN( "No instantiation is known to occur at time 15" ) {
        auto instantiations = scenario->getCurrentInstantiations(15);
        REQUIRE( instantiations.size() == 0 );
      }
      THEN( "The instantiation data of the first instance is correct" ) {
        std::string instanceId = "Instance_1";
        BPMNOS::number timestamp = 42;
        auto instantiations = scenario->getCurrentInstantiations(timestamp);
        auto& [process,values] = instantiations.front();
        REQUIRE( values.size() == 2 );
        REQUIRE( values[Model::ExtensionElements::Index::Instance].value() == BPMNOS::to_number(instanceId,STRING) );
        REQUIRE( values[Model::ExtensionElements::Index::Timestamp].value() == timestamp );
      }
    }
  }
}
