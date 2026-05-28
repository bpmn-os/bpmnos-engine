SCENARIO( "SystemState copy with undelivered message", "[systemstate][message]" ) {
  const std::string modelFile = "tests/execution/message/Simple_messaging.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A process that sends a message with no recipient instance" ) {
    // Only instantiate Process_1 (sender) - message stays undelivered
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Process_1 completed, message sent but undelivered (no recipient instance)
    REQUIRE( originalState->messages.size() == 1 );
    REQUIRE( originalState->messages.front()->waitingToken == nullptr );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The message is copied" ) {
        REQUIRE( copiedState.messages.size() == originalState->messages.size() );
      }

      THEN( "The copied message has the same values" ) {
        const auto* originalMessage = originalState->messages.front().get();
        const auto* copiedMessage = copiedState.messages.front().get();

        REQUIRE( copiedMessage->state == originalMessage->state );
        REQUIRE( copiedMessage->origin == originalMessage->origin );
        REQUIRE( copiedMessage->waitingToken == nullptr );
        REQUIRE( copiedMessage->recipient == originalMessage->recipient );
        REQUIRE( copiedMessage->header == originalMessage->header );
      }

      THEN( "The copied message is independent of the original" ) {
        REQUIRE( copiedState.messages.front().get() != originalState->messages.front().get() );
      }
    }
  }
}

SCENARIO( "SystemState copy with SendTask message awaiting delivery", "[systemstate][message][sendtask]" ) {
  const std::string modelFile = "tests/systemstate/message/Message_tasks_with_timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A SendTask in BUSY state with message awaiting delivery" ) {
    // Only instantiate Process_1 (sender) - SendTask waits for delivery
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // SendTask is in BUSY state with message awaiting delivery
    REQUIRE( originalState->messages.size() == 1 );
    REQUIRE( originalState->messages.front()->waitingToken != nullptr );
    REQUIRE( originalState->messageAwaitingDelivery.size() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The message is copied" ) {
        REQUIRE( copiedState.messages.size() == originalState->messages.size() );
      }

      THEN( "The copied message has waitingToken pointing to the copied token" ) {
        const auto* copiedMessage = copiedState.messages.front().get();

        REQUIRE( copiedMessage->waitingToken != nullptr );
        // waitingToken should point to a token in the copied state, not the original
        REQUIRE( copiedMessage->waitingToken != originalState->messages.front()->waitingToken );
        REQUIRE( copiedMessage->waitingToken->owner->systemState == &copiedState );
      }

      THEN( "The messageAwaitingDelivery container is populated" ) {
        REQUIRE( copiedState.messageAwaitingDelivery.size() == originalState->messageAwaitingDelivery.size() );

        // The key should be the copied token, not the original
        auto* copiedToken = copiedState.messages.front()->waitingToken;
        REQUIRE( copiedState.messageAwaitingDelivery.contains(copiedToken) );
        REQUIRE( copiedState.messageAwaitingDelivery.at(copiedToken).lock().get() == copiedState.messages.front().get() );
      }
    }
  }
}

SCENARIO( "SystemState copy with token awaiting boundary event", "[systemstate][message][boundary]" ) {
  const std::string modelFile = "tests/systemstate/message/Message_tasks_with_timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with a receive task and timer boundary event" ) {
    // Only instantiate Process_2 (receiver with timer boundary event)
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_2; timestamp := 0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile, csv);
    auto scenario = dataProvider.createScenario();

    Execution::Engine engine;
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    Execution::TimeWarp timeHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    timeHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    engine.run(scenario.get(), 0);
    const auto* originalState = engine.getSystemState();

    // Should have boundary event token associated with activity token
    REQUIRE( originalState->tokenAssociatedToBoundaryEventToken.size() == 1 );
    REQUIRE( originalState->tokensAwaitingBoundaryEvent.size() == 1 );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider.createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the same boundary event mappings" ) {
        REQUIRE( copiedState.tokenAssociatedToBoundaryEventToken.size() ==
                 originalState->tokenAssociatedToBoundaryEventToken.size() );
        REQUIRE( copiedState.tokensAwaitingBoundaryEvent.size() ==
                 originalState->tokensAwaitingBoundaryEvent.size() );
      }
    }
  }
}
