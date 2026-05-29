SCENARIO( "Stochastic data provider", "[data][stochastic]" ) {
  const std::string modelFile = "tests/data/stochastic/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with instantiation after disclosure time" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := 15; triangular(5,5,5);;\n"
      "Instance_1; Process_1; x := 1; triangular(10,10,10);;\n"
      "Instance_1; Activity_1; data := 5; triangular(15,15,15);;\n"
      "Instance_1; Activity_1; y := 2; triangular(20,20,20);;\n"
    ;

    WHEN( "The scenario is created" ) {
      Model::StochasticDataProvider dataProvider(modelFile, csv);
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

  GIVEN( "An instance with READY expressions changing x and y" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := 0;;;\n"
      "Instance_1; Process_1; x := 1;;;\n"
      "Instance_1; Activity_1; y := 2;;;\n"
      "Instance_1; Activity_1;;;x := triangular(100,100,100);\n"
      "Instance_1; Activity_1;;;y := triangular(200,200,200);\n"
    ;

    WHEN( "The scenario is created" ) {
      Model::StochasticDataProvider dataProvider(modelFile, csv);
      auto scenario = dataProvider.createScenario();

      auto instances = scenario->getInstances(0);
      REQUIRE( instances.size() == 1 );
      auto instance = instances[0];
      auto& process = scenario->getModel()->processes[0];
      auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
      REQUIRE( activity != nullptr );

      THEN( "x and y have initial values before noticeReadyPending" ) {
        auto processStatus = scenario->getStatus(instance->id, instance->process, 0);
        REQUIRE( processStatus.has_value() );
        REQUIRE( processStatus->at(1).value() == 1 );  // x = 1

        auto activityStatus = scenario->getStatus(instance->id, activity, 0);
        REQUIRE( activityStatus.has_value() );
        REQUIRE( activityStatus->at(0).value() == 2 );  // y = 2
      }

      THEN( "y is updated and x becomes local copy after noticeReadyPending" ) {
        // Parent status passed to activity (timestamp=0, x=1)
        Values status = {0, 1};
        Values data = {};
        Values globals = {};

        scenario->noticeReadyPending(instance->id, activity, status, data, globals);

        // Process x stays unchanged (getStatus returns disclosed values)
        auto processStatus = scenario->getStatus(instance->id, instance->process, 0);
        REQUIRE( processStatus.has_value() );
        REQUIRE( processStatus->at(1).value() == 1 );  // x = 1 (unchanged)

        // Activity ready status includes arrival expression results
        auto activityStatus = scenario->getActivityReadyStatus(instance->id, activity, 0);
        REQUIRE( activityStatus.has_value() );
        // Status: [timestamp, x, y] = [0, 100, 200]
        REQUIRE( activityStatus->at(0).value() == 0 );   // timestamp unchanged
        REQUIRE( activityStatus->at(1).value() == 100 ); // x = 100 (from arrival expr)
        REQUIRE( activityStatus->at(2).value() == 200 ); // y = 200 (from arrival expr)
      }
    }
  }

  GIVEN( "An instance with random READY expressions" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := 0;;;\n"
      "Instance_1; Process_1; x := 1;;;\n"
      "Instance_1; Activity_1; y := 2;;;\n"
      "Instance_1; Activity_1;;;x := uniform(0,1000);\n"
      "Instance_1; Activity_1;;;y := uniform(0,1000);\n"
    ;

    WHEN( "Two scenarios are created with the same seed" ) {
      Model::StochasticDataProvider dataProvider(modelFile, csv, 42);
      auto scenario1 = dataProvider.createScenario(0);
      auto scenario2 = dataProvider.createScenario(0);

      auto instances1 = scenario1->getInstances(0);
      auto instances2 = scenario2->getInstances(0);
      REQUIRE( instances1.size() == 1 );
      REQUIRE( instances2.size() == 1 );

      auto instance1 = instances1[0];
      auto instance2 = instances2[0];
      auto& process = dataProvider.getModel().processes[0];
      auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });

      Values status = {0, 1};
      Values data = {};
      Values globals = {};

      scenario1->noticeReadyPending(instance1->id, activity, status, data, globals);
      scenario2->noticeReadyPending(instance2->id, activity, status, data, globals);

      THEN( "Both scenarios produce identical arrival status" ) {
        auto activityStatus1 = scenario1->getActivityReadyStatus(instance1->id, activity, 0);
        auto activityStatus2 = scenario2->getActivityReadyStatus(instance2->id, activity, 0);

        REQUIRE( activityStatus1.has_value() );
        REQUIRE( activityStatus2.has_value() );
        REQUIRE( activityStatus1->at(1).value() == activityStatus2->at(1).value() ); // x
        REQUIRE( activityStatus1->at(2).value() == activityStatus2->at(2).value() ); // y
      }
    }

    WHEN( "Two scenarios are created with different seeds" ) {
      Model::StochasticDataProvider dataProvider(modelFile, csv, 42);
      auto scenario1 = dataProvider.createScenario(0);
      auto scenario2 = dataProvider.createScenario(1);  // different scenarioId = different seed

      auto instances1 = scenario1->getInstances(0);
      auto instances2 = scenario2->getInstances(0);

      auto instance1 = instances1[0];
      auto instance2 = instances2[0];
      auto& process = dataProvider.getModel().processes[0];
      auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });

      Values status = {0, 1};
      Values data = {};
      Values globals = {};

      scenario1->noticeReadyPending(instance1->id, activity, status, data, globals);
      scenario2->noticeReadyPending(instance2->id, activity, status, data, globals);

      THEN( "Scenarios produce different ready status" ) {
        auto activityStatus1 = scenario1->getActivityReadyStatus(instance1->id, activity, 0);
        auto activityStatus2 = scenario2->getActivityReadyStatus(instance2->id, activity, 0);

        REQUIRE( activityStatus1.has_value() );
        REQUIRE( activityStatus2.has_value() );
        // With high probability, random values will differ
        bool xDiffers = activityStatus1->at(1).value() != activityStatus2->at(1).value();
        bool yDiffers = activityStatus1->at(2).value() != activityStatus2->at(2).value();
        REQUIRE( (xDiffers || yDiffers) );
      }
    }
  }

  GIVEN( "A task with COMPLETION expressions changing x, y, z, w" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := 0;;;\n"
      "Instance_1; Process_1; x := 1;;;\n"
      "Instance_1; Activity_1; y := 2;;;\n"
      "Instance_1; Task_2; z := 3;;;\n"
      "Instance_1; Task_2;;;;x := 10\n"
      "Instance_1; Task_2;;;;y := 20\n"
      "Instance_1; Task_2;;;;z := 30\n"
      "Instance_1; Task_2;;;;w := 40\n"
      "Instance_1; Task_2;;;;timestamp := 10 + min{90,lognormal(log(10)- 0.005,0.1)}\n"
    ;

    WHEN( "The scenario is created and task runs" ) {
      Model::StochasticDataProvider dataProvider(modelFile, csv);
      auto scenario = dataProvider.createScenario();

      auto instances = scenario->getInstances(0);
      REQUIRE( instances.size() == 1 );
      auto instance = instances[0];

      auto& process = dataProvider.getModel().processes[0];
      auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
      auto task = process->find([](BPMN::Node* n) { return n->id == "Task_2"; });
      REQUIRE( activity != nullptr );
      REQUIRE( task != nullptr );

      // Status at task: [timestamp, x, y, z, w] = [0, 1, 2, 3, 4]
      Values status = {0, 1, 2, 3, 4};
      Values data = {};
      Values globals = {};

      scenario->noticeCompletionPending(instance->id, task, status, data, globals);

      THEN( "All status values are modified by completion expressions" ) {
        auto completionStatus = scenario->getTaskCompletionStatus(instance->id, task, 100);
        REQUIRE( completionStatus.has_value() );
        // Status: [timestamp, x, y, z, w] = [100, 10, 20, 30, 40]
        REQUIRE( completionStatus->at(0).value() >= 10 ); // timestamp
        REQUIRE( completionStatus->at(1).value() == 10 );  // x
        REQUIRE( completionStatus->at(2).value() == 20 );  // y
        REQUIRE( completionStatus->at(3).value() == 30 );  // z
        REQUIRE( completionStatus->at(4).value() == 40 );  // w
      }

      THEN( "Completion status not available before completion time" ) {
        auto completionStatus = scenario->getTaskCompletionStatus(instance->id, task, 9);
        REQUIRE( !completionStatus.has_value() );
      }

      THEN( "Process x remains unchanged" ) {
        auto processStatus = scenario->getStatus(instance->id, instance->process, 0);
        REQUIRE( processStatus.has_value() );
        REQUIRE( processStatus->at(1).value() == 1 );  // x = 1 (unchanged)
      }
    }
  }

  GIVEN( "READY and COMPLETION expressions with += operator" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := 0;;;\n"
      "Instance_1; Process_1; x := 100;;;\n"
      "Instance_1; Activity_1; y := 10;;;\n"
      "Instance_1; Task_2; z := 5;;;\n"
      "Instance_1; Activity_1;;;x += 50;\n"
      "Instance_1; Task_2;;;;z += 10\n"
      "Instance_1; Task_2;;;;timestamp := 10\n"
    ;

    WHEN( "The scenario is created" ) {
      Model::StochasticDataProvider dataProvider(modelFile, csv);
      auto scenario = dataProvider.createScenario();

      auto instances = scenario->getInstances(0);
      REQUIRE( instances.size() == 1 );
      auto instance = instances[0];

      auto& process = dataProvider.getModel().processes[0];
      auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
      auto task = process->find([](BPMN::Node* n) { return n->id == "Task_2"; });

      THEN( "READY expression with += operator works" ) {
        Values status = {0, 100};
        Values data = {};
        Values globals = {};

        scenario->noticeReadyPending(instance->id, activity, status, data, globals);

        auto activityStatus = scenario->getActivityReadyStatus(instance->id, activity, 0);
        REQUIRE( activityStatus.has_value() );
        REQUIRE( activityStatus->at(1).value() == 150 ); // x = 100 + 50
      }

      THEN( "COMPLETION expression with += operator works" ) {
        Values status = {0, 150, 30, 5};
        Values data = {};
        Values globals = {};

        scenario->noticeCompletionPending(instance->id, task, status, data, globals);

        auto completionStatus = scenario->getTaskCompletionStatus(instance->id, task, 100);
        REQUIRE( completionStatus.has_value() );
        REQUIRE( completionStatus->at(3).value() == 15 );  // z = 5 + 10
      }
    }
  }

}

SCENARIO( "Stochastic scenario copy constructor", "[data][stochastic][copy]" ) {
  const std::string modelFile = "tests/data/stochastic/Executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with future disclosures" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := uniform(0,10); 0;;\n"
      "Instance_1; Process_1; x := uniform(0,1000); 5;;\n"
      "Instance_1; Activity_1; y := uniform(0,1000); 15;;\n"
    ;

    WHEN( "The scenario is copied after disclosure of x" ) {
      Model::StochasticDataProvider dataProvider(modelFile, csv, 42);
      auto scenario = dataProvider.createScenario(0);
      auto* stochasticScenario = dynamic_cast<Model::StochasticScenario*>(scenario.get());
      REQUIRE( stochasticScenario != nullptr );

      // Reveal data up to time 10
      stochasticScenario->revealData(10);

      // Get original values
      auto instances = scenario->getInstances(10);
      REQUIRE( instances.size() == 1 );
      auto instance = instances[0];
      auto originalTimestamp = scenario->getValue(instance->id,
        dataProvider.getModel().processes[0]->extensionElements->as<Model::ExtensionElements>()->attributes[0].get(), 10);
      auto originalX = scenario->getValue(instance->id,
        dataProvider.getModel().processes[0]->extensionElements->as<Model::ExtensionElements>()->attributes[1].get(), 10);

      // Copy at spawnTime = 11 with new seed
      Model::StochasticScenario copiedScenario(stochasticScenario, 11, 999);

      THEN( "Past values (timestamp, x) are preserved" ) {
        auto copiedInstances = copiedScenario.getInstances(10);
        REQUIRE( copiedInstances.size() == 1 );
        auto copiedInstance = copiedInstances[0];

        auto copiedTimestamp = copiedScenario.getValue(copiedInstance->id,
          dataProvider.getModel().processes[0]->extensionElements->as<Model::ExtensionElements>()->attributes[0].get(), 10);
        auto copiedX = copiedScenario.getValue(copiedInstance->id,
          dataProvider.getModel().processes[0]->extensionElements->as<Model::ExtensionElements>()->attributes[1].get(), 10);

        REQUIRE( originalTimestamp.has_value() );
        REQUIRE( copiedTimestamp.has_value() );
        REQUIRE( originalTimestamp.value() == copiedTimestamp.value() );

        REQUIRE( originalX.has_value() );
        REQUIRE( copiedX.has_value() );
        REQUIRE( originalX.value() == copiedX.value() );
      }

      THEN( "Future values (y) are resampled" ) {
        // y has disclosure time 15, so it should be resampled
        auto& process = dataProvider.getModel().processes[0];
        auto activity = process->find([](BPMN::Node* n) { return n->id == "Activity_1"; });
        REQUIRE( activity != nullptr );

        // Get y from original at time 15
        auto originalY = scenario->getStatus(instance->id, activity, 15);
        REQUIRE( originalY.has_value() );

        // Keep copying with different seeds until we get a different value
        unsigned int seed = 1;
        bool foundDifferent = false;
        while (!foundDifferent && seed < 1000) {
          Model::StochasticScenario testCopy(stochasticScenario, 11, seed);
          auto copiedY = testCopy.getStatus(instance->id, activity, 15);
          REQUIRE( copiedY.has_value() );
          if (originalY->at(0).value() != copiedY->at(0).value()) {
            foundDifferent = true;
          }
          ++seed;
        }
        REQUIRE( foundDifferent );
      }
    }
  }

  GIVEN( "An instance with all disclosures in the past" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := 0; 0;;\n"
      "Instance_1; Process_1; x := 42; 0;;\n"
    ;

    WHEN( "The scenario is copied" ) {
      Model::StochasticDataProvider dataProvider(modelFile, csv, 42);
      auto scenario = dataProvider.createScenario(0);
      auto* stochasticScenario = dynamic_cast<Model::StochasticScenario*>(scenario.get());

      // Reveal all data
      stochasticScenario->revealData(10);

      // Copy at spawnTime = 11
      Model::StochasticScenario copiedScenario(stochasticScenario, 11, 999);

      THEN( "All values are preserved" ) {
        auto copiedInstances = copiedScenario.getInstances(0);
        REQUIRE( copiedInstances.size() == 1 );
        auto copiedInstance = copiedInstances[0];

        auto copiedX = copiedScenario.getValue(copiedInstance->id,
          dataProvider.getModel().processes[0]->extensionElements->as<Model::ExtensionElements>()->attributes[1].get(), 0);
        REQUIRE( copiedX.has_value() );
        REQUIRE( copiedX.value() == 42 );
      }
    }
  }

}
