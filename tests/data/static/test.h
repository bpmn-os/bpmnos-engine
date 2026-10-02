SCENARIO( "Trivial executable process", "[data][static]" ) {
  const std::string modelFile = "tests/data/static/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n";

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
          REQUIRE( BPMNOS::to_string(instance->id,STRING) == "Instance_1" );
          auto extensionElements = instance->process->extensionElements->represents<Model::ExtensionElements>();
          REQUIRE( extensionElements->attributes.size() == 1 );
//          REQUIRE( (std::string)extensionElements->attributes[Model::ExtensionElements::Index::Instance]->id == Keyword::Instance );
//          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Instance]->isImmutable == true );
//          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Instance]->value == std::nullopt );
          REQUIRE( (std::string)extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->id == Keyword::Timestamp );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->isImmutable == false );
        }
      }
      THEN( "The instantiation data is correct" ) {
        std::string instanceId = "Instance_1";
        BPMNOS::number timestamp = 0;
        auto instantiations = scenario->getCurrentInstantiations(0);
        REQUIRE( instantiations.size() == 1 );
        auto& [process,status,data] = instantiations.front();
        REQUIRE( status.size() == 1 );
        REQUIRE( data.size() == 1 );
        REQUIRE( data[Model::ExtensionElements::Index::Instance].value() == BPMNOS::to_number(instanceId,STRING) );
        REQUIRE( status[Model::ExtensionElements::Index::Timestamp].value() == timestamp );
      }
    }
  }
  GIVEN( "A single instance with timestamp input" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 42\n"
    ;

    WHEN( "The instance is loaded" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      THEN( "There earliest instantiation is at time 42" ) {
        REQUIRE( scenario->getEarliestInstantiationTime() == 42 );
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
        auto anticipatedInstances = scenario->getInstances(42);
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
        auto instances = scenario->getInstances(0);
        for ( auto& instance : instances ) {
          auto extensionElements = instance->process->extensionElements->represents<Model::ExtensionElements>();
          REQUIRE( extensionElements->attributes.size() == 1 );
//          REQUIRE( (std::string)extensionElements->attributes[Model::ExtensionElements::Index::Instance]->id == Keyword::Instance );
//          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Instance]->isImmutable == true );
//          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Instance]->value == std::nullopt );
          REQUIRE( (std::string)extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->id == Keyword::Timestamp );
          REQUIRE( extensionElements->attributes[Model::ExtensionElements::Index::Timestamp]->isImmutable == false );
        }
      }
      THEN( "The instantiation data of the first instance is correct" ) {
        std::string instanceId = "Instance_1";
        BPMNOS::number timestamp = 42;
        auto instantiations = scenario->getCurrentInstantiations(timestamp);
        auto& [process,status,data] = instantiations.front();
        REQUIRE( status.size() == 1 );
        REQUIRE( data.size() == 1 );
        REQUIRE( data[Model::ExtensionElements::Index::Instance].value() == BPMNOS::to_number(instanceId,STRING) );
        REQUIRE( status[Model::ExtensionElements::Index::Timestamp].value() == timestamp );
      }
    }
  }
  GIVEN( "Two instances, one with instance and timestamp input" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Process_1; timestamp := 42\n"
      "Instance_2; Process_1;\n"
    ;

    WHEN( "The instance is loaded" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);

      auto scenario = dataProvider.createScenario();
      THEN( "There earliest instantiation is at time 0" ) {
        REQUIRE( scenario->getEarliestInstantiationTime() == 0 );
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
        auto anticipatedInstances = scenario->getInstances(0);
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
        auto& [process,status,data] = instantiations.front();
        REQUIRE( status.size() == 1 );
        REQUIRE( data.size() == 1 );
        REQUIRE( data[Model::ExtensionElements::Index::Instance].value() == BPMNOS::to_number(instanceId,STRING) );
        REQUIRE( status[Model::ExtensionElements::Index::Timestamp].value() == timestamp );
      }
    }
  }
}

SCENARIO( "Known and ready instances of a static scenario", "[data][static]" ) {
  const std::string modelFile = "tests/data/static/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "Two instances with different instantiation times" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_1; timestamp := 42\n"
    ;
    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The known instances are queried at every instant of a run starting at time 0" ) {
      std::map<BPMNOS::number, BPMNOS::number> reported; // instance -> time at which it is reported
      bool reportedTwice = false;
      auto previous = std::numeric_limits<BPMNOS::number>::lowest();
      for ( BPMNOS::number t = 0; t <= 50; t = t + 1 ) {
        for ( auto& [process,status,data] : scenario->getKnownInstantiations(previous, t) ) {
          auto instanceId = data[Model::ExtensionElements::Index::Instance].value();
          reportedTwice = reportedTwice || reported.contains(instanceId);
          reported[instanceId] = t;
        }
        previous = t;
      }
      THEN( "Every instance is reported exactly once, by the first call" ) {
        REQUIRE( !reportedTwice );
        REQUIRE( reported.size() == 2 );
        REQUIRE( reported.at(BPMNOS::to_number(std::string("Instance_1"),STRING)) == 0 );
        REQUIRE( reported.at(BPMNOS::to_number(std::string("Instance_2"),STRING)) == 0 );
      }
    }

    WHEN( "The ready status is queried" ) {
      auto instanceId = BPMNOS::to_number(std::string("Instance_2"),STRING);
      THEN( "It is absent before the instantiation time" ) {
        REQUIRE( !scenario->getProcessReadyStatus(instanceId, 41).has_value() );
      }
      THEN( "It equals the instantiation at the instantiation time" ) {
        auto instantiations = scenario->getCurrentInstantiations(42);
        REQUIRE( instantiations.size() == 1 );
        auto& [process,status,data] = instantiations.front();
        auto readyStatus = scenario->getProcessReadyStatus(instanceId, 42);
        REQUIRE( readyStatus.has_value() );
        REQUIRE( readyStatus.value() == status );
        auto readyData = scenario->getData(instanceId, process, 42);
        REQUIRE( readyData.has_value() );
        REQUIRE( readyData.value() == data );
      }
    }
  }
}
