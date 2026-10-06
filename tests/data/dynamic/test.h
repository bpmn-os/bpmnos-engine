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
        auto instances = scenario->getInstances(5);
        REQUIRE( instances.size() == 0 );
      }
      THEN( "Instance is known at disclosure time of x" ) {
        auto instances = scenario->getInstances(10);
        REQUIRE( instances.size() == 1 );
      }
      THEN( "data and y are not known before its disclosure time" ) {
        auto instances = scenario->getInstances(10);
        REQUIRE( instances.size() == 1 );
        auto instance = instances[0];
        auto& process = scenario->getModel()->processes[0];
        auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
        REQUIRE( activity != nullptr );
        
        auto data = scenario->getData(instance->id, activity, 10);
        REQUIRE( !data.has_value() );
        auto status = scenario->getStatus(instance->id, activity, 10);
        REQUIRE( !status.has_value() );
      }
      THEN( "data is not disclosed before y" ) {
        auto instances = scenario->getInstances(10);
        REQUIRE( instances.size() == 1 );
        auto instance = instances[0];
        auto& process = scenario->getModel()->processes[0];
        auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
        REQUIRE( activity != nullptr );

        auto data = scenario->getData(instance->id, activity, 15);
        REQUIRE( !data.has_value() );
        auto status = scenario->getStatus(instance->id, activity, 15);
        REQUIRE( !status.has_value() );
      }
      THEN( "data and y are disclosed eventually" ) {
        auto instances = scenario->getInstances(10);
        REQUIRE( instances.size() == 1 );
        auto instance = instances[0];
        auto& process = scenario->getModel()->processes[0];
        auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
        REQUIRE( activity != nullptr );

        auto data = scenario->getData(instance->id, activity, 20);
        REQUIRE( data.has_value() );
        auto status = scenario->getStatus(instance->id, activity, 20);
        REQUIRE( status.has_value() );
      }
    }
  }

}

SCENARIO( "Known and ready instances of a dynamic scenario", "[data][dynamic]" ) {
  const std::string modelFile = "tests/data/dynamic/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "Instances disclosed before, after, and at the outset of their instantiation" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 15; 5\n"
      "Instance_2; Process_1; timestamp := 5; 10\n"
      "Instance_3; Process_1; timestamp := 20; 0\n"
    ;
    Model::DynamicDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The known instances are queried at every instant of a run starting at time 3" ) {
      std::map<BPMNOS::number, BPMNOS::number> reported; // instance -> time at which it is reported
      bool reportedTwice = false;
      auto previous = std::numeric_limits<BPMNOS::number>::lowest();
      for ( BPMNOS::number t = 3; t <= 30; t = t + 1 ) {
        for ( auto& [process,status,data] : scenario->getKnownInstantiations(previous, t) ) {
          auto instanceId = data[Model::ExtensionElements::Index::Instance].value();
          reportedTwice = reportedTwice || reported.contains(instanceId);
          reported[instanceId] = t;
        }
        previous = t;
      }
      THEN( "Every instance is reported exactly once, at its disclosure or by the first call" ) {
        REQUIRE( !reportedTwice );
        REQUIRE( reported.size() == 3 );
        REQUIRE( reported.at(BPMNOS::to_number(std::string("Instance_1"),STRING)) == 5 );
        REQUIRE( reported.at(BPMNOS::to_number(std::string("Instance_2"),STRING)) == 10 );
        REQUIRE( reported.at(BPMNOS::to_number(std::string("Instance_3"),STRING)) == 3 );
      }
    }

    WHEN( "The ready status is queried" ) {
      THEN( "It is absent before the effective instantiation time" ) {
        REQUIRE( !scenario->getProcessReadyStatus(BPMNOS::to_number(std::string("Instance_1"),STRING), 14).has_value() );
        REQUIRE( !scenario->getProcessReadyStatus(BPMNOS::to_number(std::string("Instance_2"),STRING), 9).has_value() );
        REQUIRE( !scenario->getProcessReadyStatus(BPMNOS::to_number(std::string("Instance_3"),STRING), 19).has_value() );
      }
      THEN( "It is given at the effective instantiation time together with the data of the process" ) {
        auto process = scenario->getModel()->processes.front().get();
        for ( auto [name, t] : std::vector< std::pair<std::string, int> >{ {"Instance_1", 15}, {"Instance_2", 10}, {"Instance_3", 20} } ) {
          auto instanceId = BPMNOS::to_number(name,STRING);
          auto readyStatus = scenario->getProcessReadyStatus(instanceId, t);
          REQUIRE( readyStatus.has_value() );
          REQUIRE( readyStatus.value()[Model::ExtensionElements::Index::Timestamp].value() == t );
          auto readyData = scenario->getData(instanceId, process, t);
          REQUIRE( readyData.has_value() );
          REQUIRE( readyData.value()[Model::ExtensionElements::Index::Instance].value() == instanceId );
        }
      }
    }
  }
}

SCENARIO( "Dynamic data for scopes created by the engine", "[data][dynamic]" ) {
  GIVEN( "A value for an attribute of an event subprocess" ) {
    const std::string modelFile = "tests/execution/eventsubprocess/Non-interrupting_escalation.bpmn";
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1;;\n"
      "Instance_1; EventSubProcess_1; timestamp := 1; 0\n"
    ;
    THEN( "The data provider rejects it" ) {
      REQUIRE_THROWS( Model::DynamicDataProvider(modelFile,csv) );
    }
  }
  GIVEN( "A value for an attribute of a compensation activity" ) {
    const std::string modelFile = "tests/execution/compensationactivity/Compensation_task.bpmn";
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1;;\n"
      "Instance_1; CompensationActivity_1; timestamp := 1; 0\n"
    ;
    THEN( "The data provider rejects it" ) {
      REQUIRE_THROWS( Model::DynamicDataProvider(modelFile,csv) );
    }
  }
}

SCENARIO( "Dynamic data for guidance attributes", "[data][dynamic]" ) {
  GIVEN( "A value for an attribute of a guidance" ) {
    const std::string modelFile = "examples/bin_packing_problem/Bin_packing_problem.bpmn";
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Bin1; BinProcess; capacity := 40.0;\n"
      "Bin1; CatchRequestMessage; fill_rate := 0.5; 0\n"
    ;
    THEN( "The data provider rejects it" ) {
      REQUIRE_THROWS_WITH( Model::DynamicDataProvider(modelFile,csv), Catch::Matchers::ContainsSubstring("guidance") );
    }
  }
}
