// Tests that the Scenario stores the correct completion/arrival status for a task — in particular the timestamp,
// which the task completion handler compares against the current time to decide when to fire a CompletionEvent.
// A wrong (e.g. spuriously large) timestamp here makes the task never complete and the engine clock-tick.

SCENARIO( "Scenario records the correct completion timestamp for a BUSY task", "[candidates][scenario][timestamp]" ) {
  const std::string modelFile = "tests/execution/task/Task_with_linear_expression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A task that enters at t=0 and is BUSY with its completion scheduled at t=1" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
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
    engine.run(scenario.get(), 0);

    // Resolve (instanceId, node) from the scenario's own model so the keys match those the scenario stores.
    const BPMN::Node* node = nullptr;
    for ( auto& process : scenario->model->processes ) {
      for ( auto* flowNode : process->flowNodes ) {
        if ( flowNode->id == "Activity_1" ) node = flowNode;
      }
    }
    REQUIRE( node != nullptr );
    REQUIRE_FALSE( engine.getSystemState()->instances.empty() );
    auto instanceId = engine.getSystemState()->instances.front()->instance.value();
    constexpr auto Timestamp = BPMNOS::Model::ExtensionElements::Index::Timestamp;

    THEN( "Scenario::taskCompletionStatus holds the completion at t=1, and not before" ) {
      REQUIRE_FALSE( scenario->getTaskCompletionStatus(instanceId, node, 0).has_value() );
      auto completion = scenario->getTaskCompletionStatus(instanceId, node, 1);
      REQUIRE( completion.has_value() );
      REQUIRE( (*completion)[Timestamp].has_value() );
      REQUIRE( (double)(*completion)[Timestamp].value() == 1.0 );
    }
    AND_THEN( "Scenario::activityArrivalStatus exposes the activity as ready at its arrival time t=0" ) {
      auto ready = scenario->getActivityReadyStatus(instanceId, node, 0);
      REQUIRE( ready.has_value() );
      REQUIRE( (*ready)[Timestamp].has_value() );
      REQUIRE( (double)(*ready)[Timestamp].value() == 0.0 );
    }
  }
}
