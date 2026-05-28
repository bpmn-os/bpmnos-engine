SCENARIO( "SystemState copy with pending event subprocess", "[systemstate][eventsubprocess]" ) {
  const std::string modelFile = "tests/systemstate/eventsubprocess/Untriggered_evt_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with a pending event subprocess" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    // No entry handler - Activity_1 blocks, escalation never thrown
    Execution::TimeWarp timeHandler;
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Find the context StateMachine (owned by process token)
    const Execution::StateMachine* originalContext = nullptr;
    for (const auto& instance : originalState->instances) {
      for (const auto& processToken : instance->tokens) {
        if (processToken->owned) {
          originalContext = processToken->owned.get();
          break;
        }
      }
      if (originalContext) break;
    }
    REQUIRE( originalContext != nullptr );
    REQUIRE( originalContext->pendingEventSubProcesses.size() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same pending event subprocess" ) {
        const Execution::StateMachine* copiedContext = nullptr;
        for (const auto& instance : copiedState.instances) {
          for (const auto& processToken : instance->tokens) {
            if (processToken->owned) {
              copiedContext = processToken->owned.get();
              break;
            }
          }
          if (copiedContext) break;
        }
        REQUIRE( copiedContext != nullptr );
        REQUIRE( copiedContext->pendingEventSubProcesses.size() ==
                 originalContext->pendingEventSubProcesses.size() );

        // Verify the pending event subprocess has a token at TypedStartEvent in BUSY state
        REQUIRE( copiedContext->pendingEventSubProcesses.front()->tokens.size() == 1 );
        REQUIRE( copiedContext->pendingEventSubProcesses.front()->tokens.front()->state ==
                 Execution::Token::State::BUSY );
      }
    }
  }
}

SCENARIO( "SystemState copy with interrupting event subprocess", "[systemstate][eventsubprocess]" ) {
  const std::string modelFile = "tests/systemstate/eventsubprocess/Triggered_interrupting_evt_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with an active interrupting event subprocess" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    // No exit handler - Activity_1 in event subprocess stays BUSY
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Find the context StateMachine
    const Execution::StateMachine* originalContext = nullptr;
    for (const auto& instance : originalState->instances) {
      for (const auto& processToken : instance->tokens) {
        if (processToken->owned) {
          originalContext = processToken->owned.get();
          break;
        }
      }
      if (originalContext) break;
    }
    REQUIRE( originalContext != nullptr );
    REQUIRE( originalContext->interruptingEventSubProcess != nullptr );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the interrupting event subprocess" ) {
        const Execution::StateMachine* copiedContext = nullptr;
        for (const auto& instance : copiedState.instances) {
          for (const auto& processToken : instance->tokens) {
            if (processToken->owned) {
              copiedContext = processToken->owned.get();
              break;
            }
          }
          if (copiedContext) break;
        }
        REQUIRE( copiedContext != nullptr );
        REQUIRE( copiedContext->interruptingEventSubProcess != nullptr );
        REQUIRE( copiedContext->interruptingEventSubProcess.get() !=
                 originalContext->interruptingEventSubProcess.get() );
        REQUIRE( copiedContext->interruptingEventSubProcess->scope ==
                 originalContext->interruptingEventSubProcess->scope );
      }
    }
  }
}

SCENARIO( "SystemState copy with non-interrupting event subprocess", "[systemstate][eventsubprocess]" ) {
  const std::string modelFile = "tests/systemstate/eventsubprocess/Triggered_non-interrupting_evt_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with an active non-interrupting event subprocess" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    // No exit handler - Activity_1 in event subprocess stays BUSY
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Find the context StateMachine
    const Execution::StateMachine* originalContext = nullptr;
    for (const auto& instance : originalState->instances) {
      for (const auto& processToken : instance->tokens) {
        if (processToken->owned) {
          originalContext = processToken->owned.get();
          break;
        }
      }
      if (originalContext) break;
    }
    REQUIRE( originalContext != nullptr );
    REQUIRE( originalContext->nonInterruptingEventSubProcesses.size() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the non-interrupting event subprocess" ) {
        const Execution::StateMachine* copiedContext = nullptr;
        for (const auto& instance : copiedState.instances) {
          for (const auto& processToken : instance->tokens) {
            if (processToken->owned) {
              copiedContext = processToken->owned.get();
              break;
            }
          }
          if (copiedContext) break;
        }
        REQUIRE( copiedContext != nullptr );
        REQUIRE( copiedContext->nonInterruptingEventSubProcesses.size() ==
                 originalContext->nonInterruptingEventSubProcesses.size() );
        REQUIRE( copiedContext->nonInterruptingEventSubProcesses.front().get() !=
                 originalContext->nonInterruptingEventSubProcesses.front().get() );
        REQUIRE( copiedContext->nonInterruptingEventSubProcesses.front()->scope ==
                 originalContext->nonInterruptingEventSubProcesses.front()->scope );
      }
    }
  }
}
