#include "prelude.h"

namespace {

/// An engine entering every activity at once, which keeps the final system state of its run.
struct Run {
  Execution::Engine engine;
  Execution::InstantEntry entryHandler;

  /// Runs a scenario of the data provider until its end time.
  Run(std::shared_ptr<const Model::Model> model, Execution::StaticDataProvider& dataProvider, BPMNOS::number endTime)
    : engine(model)
  {
    entryHandler.connect(&engine);
    dataProvider.setEndTime(endTime);
    engine.run(dataProvider.createScenario(), 0);
  }
};

} // namespace

SCENARIO( "A system state awaits the events of its tokens", "[systemstate][awaiting]" ) {

  GIVEN( "A token at a busy task" ) {
    auto model = std::make_shared<const Model::Model>("tests/systemstate/task/Task_with_linear_expression.bpmn");
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    );
    Run run(model, *dataProvider, 0);
    REQUIRE( run.engine.getSystemState()->tokensAwaitingCompletionEvent.count() == 1 );
    THEN( "The system state awaits events" ) {
      REQUIRE( run.engine.getSystemState()->isAwaitingEvents() );
    }
  }

  GIVEN( "A token awaiting a timer" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/timer/Timer.bpmn");
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger := 10\n"
    );
    Run run(model, *dataProvider, 0);
    REQUIRE( run.engine.getSystemState()->tokensAwaitingTimer.count() == 1 );
    THEN( "The system state awaits events" ) {
      REQUIRE( run.engine.getSystemState()->isAwaitingEvents() );
    }
  }

  GIVEN( "A token awaiting its ready event" ) {
    auto model = std::make_shared<const Model::Model>("tests/systemstate/process/Executable_process.bpmn");
    auto dataProvider = std::make_shared<Execution::DynamicDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 0; 0\n"
      "Instance_1; Activity_1; y := 0; 10\n"
      "Instance_1; Activity_1; data := 0; 10\n"
    );
    Run run(model, *dataProvider, 0);
    REQUIRE( run.engine.getSystemState()->tokensAwaitingReadyEvent.count() == 1 );
    THEN( "The system state awaits events" ) {
      REQUIRE( run.engine.getSystemState()->isAwaitingEvents() );
    }
  }

  GIVEN( "Tokens awaiting entry only" ) {
    auto model = std::make_shared<const Model::Model>("tests/systemstate/process/Simple_executable_process.bpmn");
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_1; timestamp := 0\n"
    );
    // the engine enters the tasks itself in the round after they become ready, so that the run is advanced round
    // by round until both await entry
    auto awaitingEntry = [](const Execution::SystemState* systemState) {
      size_t count = 0;
      for ( auto& [instanceId, waitingTokens] : systemState->tokensAwaitingEntry ) {
        count += waitingTokens.count();
      }
      return count;
    };
    Execution::Engine engine(model);
    engine.initialize(dataProvider->createScenario(), 0);
    while ( awaitingEntry(engine.getSystemState()) < 2 ) {
      REQUIRE( engine.advance() );
    }
    THEN( "The system state awaits no event" ) {
      REQUIRE( !engine.getSystemState()->isAwaitingEvents() );
    }
  }

  GIVEN( "A run that has ended" ) {
    auto model = std::make_shared<const Model::Model>("tests/systemstate/process/Simple_executable_process.bpmn");
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    );
    Run run(model, *dataProvider, std::numeric_limits<BPMNOS::number>::max());
    THEN( "The system state awaits no event" ) {
      REQUIRE( !run.engine.getSystemState()->isAwaitingEvents() );
    }
  }
}

SCENARIO( "A data provider expects the instances not yet instantiated", "[data][awaiting]" ) {
  auto model = std::make_shared<const Model::Model>("tests/systemstate/process/Simple_executable_process.bpmn");

  GIVEN( "A static data provider" ) {
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_1; timestamp := 10\n"
    );
    Run run(model, *dataProvider, 0);
    THEN( "It expects no data, every instance being instantiated at the start" ) {
      REQUIRE( !dataProvider->expectsData(*run.engine.getSystemState()->scenario) );
    }
  }

  GIVEN( "A dynamic data provider with an instance disclosed at time 10" ) {
    auto dataProvider = std::make_shared<Execution::DynamicDataProvider>(model,
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 0; 0\n"
      "Instance_2; Process_1; timestamp := 0; 10\n"
    );

    WHEN( "The run ends before the instance is instantiated" ) {
      Run run(model, *dataProvider, 5);
      REQUIRE( run.engine.getSystemState()->getTime() < 10 );
      THEN( "The data provider expects data" ) {
        REQUIRE( dataProvider->expectsData(*run.engine.getSystemState()->scenario) );
      }
    }

    WHEN( "The run ends after the instance is instantiated" ) {
      Run run(model, *dataProvider, 10);
      REQUIRE( run.engine.getSystemState()->getTime() == 10 );
      THEN( "The data provider expects no data" ) {
        REQUIRE( !dataProvider->expectsData(*run.engine.getSystemState()->scenario) );
      }
    }
  }
}
