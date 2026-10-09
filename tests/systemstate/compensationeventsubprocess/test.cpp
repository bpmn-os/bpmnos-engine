#include "prelude.h"

SCENARIO( "SystemState copy with compensable subprocess", "[systemstate][compensationeventsubprocess]" ) {
  const std::string modelFile = "tests/systemstate/compensationeventsubprocess/Simple_compensation_event_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with a compensable subprocess" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    // Run until time 5 - SubProcess_1 completed, timer waiting
    dataProvider->setEndTime(5);
    engine.run(std::move(scenario), 0);
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
    REQUIRE( originalContext->compensableSubProcesses.size() == 1 );

    // Verify the compensable subprocess token has an owned StateMachine
    const auto& compensableToken = originalContext->compensableSubProcesses.front();
    REQUIRE( compensableToken->owned != nullptr );
    REQUIRE( compensableToken->owned->scope->id == "SubProcess_1" );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider->createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the compensable subprocess" ) {
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
        REQUIRE( copiedContext->compensableSubProcesses.size() ==
                 originalContext->compensableSubProcesses.size() );

        // Verify the copied token has an owned StateMachine
        const auto& copiedCompensableToken = copiedContext->compensableSubProcesses.front();
        REQUIRE( copiedCompensableToken->owned != nullptr );
        REQUIRE( copiedCompensableToken->owned->scope ==
                 compensableToken->owned->scope );

        // Verify it's a different object (deep copy)
        REQUIRE( copiedCompensableToken.get() != compensableToken.get() );
        REQUIRE( copiedCompensableToken->owned.get() != compensableToken->owned.get() );

        // Verify parentToken points to the token copy
        REQUIRE( copiedCompensableToken->owned->parentToken == copiedCompensableToken.get() );
      }
    }
  }
}

SCENARIO( "SystemState copy with active compensation event subprocess", "[systemstate][compensationeventsubprocess]" ) {
  const std::string modelFile = "tests/systemstate/compensationeventsubprocess/Simple_compensation_event_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance with compensation triggered" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
//    Execution::Recorder recorder(std::cerr);
    recorder.subscribe(&engine);

    // Run until time 15 - timer at 10 fired, compensation triggered, waiting at timer 20
    dataProvider->setEndTime(15);
    engine.run(std::move(scenario), 0);
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
    REQUIRE( originalContext->compensationEventSubProcesses.size() == 1 );

    // Verify tokenAwaitingCompensationEventSubProcess is populated
    auto* originalCompensationEventSubProcess = originalContext->compensationEventSubProcesses.front().get();
    auto originalIt = originalState->tokenAwaitingCompensationEventSubProcess.find(originalCompensationEventSubProcess);
    REQUIRE( originalIt != originalState->tokenAwaitingCompensationEventSubProcess.end() );
    REQUIRE( originalIt->second->node->id == "CompensateThrowEvent_1" );

    WHEN( "SystemState is copied" ) {
      auto scenarioCopy = dataProvider->createScenario();
      Execution::SystemState copiedState(&engine, scenarioCopy.get(), originalState);

      THEN( "The copy has the compensation event subprocess" ) {
        // Find copied context StateMachine
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
        REQUIRE( copiedContext->compensationEventSubProcesses.size() ==
                 originalContext->compensationEventSubProcesses.size() );

        // Verify it's a different object (deep copy)
        REQUIRE( copiedContext->compensationEventSubProcesses.front().get() !=
                 originalContext->compensationEventSubProcesses.front().get() );

        // Verify scope is the same
        REQUIRE( copiedContext->compensationEventSubProcesses.front()->scope ==
                 originalContext->compensationEventSubProcesses.front()->scope );

        // Verify tokenAwaitingCompensationEventSubProcess is copied
        auto* copiedCompensationEventSubProcess = copiedContext->compensationEventSubProcesses.front().get();
        auto copiedIt = copiedState.tokenAwaitingCompensationEventSubProcess.find(copiedCompensationEventSubProcess);
        REQUIRE( copiedIt != copiedState.tokenAwaitingCompensationEventSubProcess.end() );
        REQUIRE( copiedIt->second->node->id == "CompensateThrowEvent_1" );

        // Verify it points to the copied token, not the original
        REQUIRE( copiedIt->second != originalIt->second );
      }
    }
  }
}
