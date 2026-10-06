#include <chrono>

/**
 * Test data provider enqueuing, in addition to the events of the legacy data provider, a termination event
 * once the given time is reached.
 */
class TestDataProvider : public Execution::LegacyDataProvider {
public:
  TestDataProvider(const Model::Model* model, unsigned int clockTickDuration, std::optional<BPMNOS::number> terminationTime = std::nullopt)
    : LegacyDataProvider(model, clockTickDuration)
    , terminationTime(terminationTime)
  {
  }

  void dispatchEvent(const Execution::SystemState* systemState, Execution::Scenario& scenario, Execution::EventQueue& queue) const override {
    if ( terminationTime.has_value() && systemState->getTime() >= terminationTime.value() ) {
      queue.push_back(std::make_shared<Execution::TerminationEvent>());
      return;
    }
    LegacyDataProvider::dispatchEvent(systemState, scenario, queue);
  }

  const std::optional<BPMNOS::number> terminationTime;
};

SCENARIO( "Clock ticks and termination supplied by the data provider", "[data][provider]" ) {
  const std::string modelFile = "tests/execution/timer/Timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance whose timer is triggered at time 10" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger := 10\n"
    ;
    Model::StaticDataProvider modelDataProvider(modelFile,csv);
    auto modelScenario = modelDataProvider.createScenario();

    // the reference run, in which the clock ticks are supplied by a time warp
    Execution::Engine referenceEngine;
    Execution::InstantEntry referenceEntryHandler;
    Execution::InstantExit referenceExitHandler;
    Execution::TimeWarp timeHandler;
    referenceEntryHandler.connect(&referenceEngine);
    referenceExitHandler.connect(&referenceEngine);
    timeHandler.connect(&referenceEngine);
    Execution::Recorder referenceRecorder;
    referenceRecorder.subscribe(&referenceEngine);
    referenceEngine.run(modelScenario.get());

    WHEN( "The engine runs without a clock dispatcher on a data provider without clock tick duration" ) {
      auto dataProvider = std::make_shared<const TestDataProvider>(modelScenario->getModel(), 0);
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario(modelScenario.get()));

      THEN( "Every decision of the controller precedes the clock tick following it, as with a time warp" ) {
        REQUIRE( recorder.log == referenceRecorder.log );
      }
      THEN( "The run ends exactly when the termination event is processed" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.size() == 1 );
      }
    }

    WHEN( "The engine runs without a clock dispatcher on a data provider with a clock tick duration" ) {
      const unsigned int clockTickDuration = 20;
      auto dataProvider = std::make_shared<const TestDataProvider>(modelScenario->getModel(), clockTickDuration);
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      auto scenario = dataProvider->createScenario(modelScenario.get());
      auto start = std::chrono::steady_clock::now();
      engine.run(std::move(scenario));
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

      THEN( "Every decision of the controller is processed before the next clock tick, as with a time warp" ) {
        REQUIRE( recorder.log == referenceRecorder.log );
      }
      THEN( "The clock ticks follow the wall clock" ) {
        // the opening clock tick is processed by the engine, every later one is due the given duration
        // after the previous one
        auto clockTicks = recorder.find(nlohmann::json{{"event","clocktick"}}).size();
        REQUIRE( clockTicks > 2 );
        REQUIRE( (size_t)elapsed >= ( clockTicks - 2 ) * clockTickDuration );
      }
    }

    WHEN( "The engine runs on a data provider terminating the run at time 5" ) {
      auto dataProvider = std::make_shared<const TestDataProvider>(modelScenario->getModel(), 0, 5);
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario(modelScenario.get()));

      THEN( "The run ends when the termination event is processed, with the instance still running" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        REQUIRE( engine.getCurrentTime() == 5 );
        REQUIRE( engine.getSystemState()->instances.size() == 1 );
        auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}});
        REQUIRE( timerLog.empty() );
      }
    }
  }
}

SCENARIO( "No clock tick before the first request to the controller after installing a system state", "[data][provider]" ) {
  const std::string modelFile = "tests/execution/task/Task_with_linear_expression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A system state in which a token awaits the decision to enter a task" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    Model::StaticDataProvider modelDataProvider(modelFile,csv);
    auto modelScenario = modelDataProvider.createScenario();

    // without an entry handler the token awaits the entry decision, and the run ends at time 0
    Execution::Engine sourceEngine;
    sourceEngine.run(modelScenario.get(), 0, 0);
    REQUIRE_FALSE( sourceEngine.getSystemState()->pendingEntryDecisions.empty() );

    WHEN( "The system state is installed in an engine without a clock dispatcher and the engine advances" ) {
      auto dataProvider = std::make_shared<const TestDataProvider>(modelScenario->getModel(), 0);
      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.initializeSystemState(dataProvider->createScenario(modelScenario.get()), sourceEngine.getSystemState());
      while ( recorder.find(nlohmann::json{{"event","entry"}}).empty() && recorder.find(nlohmann::json{{"event","clocktick"}}).empty() ) {
        REQUIRE( engine.advance() );
      }

      THEN( "The entry event is processed before any clock tick" ) {
        REQUIRE_FALSE( recorder.find(nlohmann::json{{"event","entry"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"event","clocktick"}}).empty() );
      }
    }
  }
}

/**
 * Data provider deriving from the base directly, which instantiates a single instance of a process at time
 * zero, starts it at once, raises a signal at a given time, and ends the run at a given time.
 */
class ScriptedDataProvider : public Execution::DataProvider {
public:
  class Scenario : public Execution::Scenario {
  public:
    Scenario(std::shared_ptr<const ScriptedDataProvider> dataProvider)
      : Execution::Scenario(std::move(dataProvider))
    {
    }
    bool instantiated = false;
    bool signalRaised = false;
    std::list< std::weak_ptr<const Execution::Token> > tokensAwaitingReadyEvent;
  };

  ScriptedDataProvider(std::shared_ptr<const Model::Model> model, const BPMN::Process* process, std::string instanceId, BPMNOS::number signalTime, BPMNOS::VariedValueMap signalContent, BPMNOS::number terminationTime)
    : DataProvider(model.get())
    , model(std::move(model))
    , process(process)
    , instanceId(BPMNOS::to_number(instanceId,STRING))
    , signalTime(signalTime)
    , signalContent(std::move(signalContent))
    , terminationTime(terminationTime)
  {
  }

  std::unique_ptr<Scenario> createScenario() const {
    return std::make_unique<Scenario>(std::static_pointer_cast<const ScriptedDataProvider>(shared_from_this()));
  }

  void notice(const Execution::Observable* observable, Execution::Scenario& scenario, [[maybe_unused]] Execution::EventQueue& queue) const override {
    if ( observable->getObservableType() == Execution::Observable::Type::Token ) {
      auto token = static_cast<const Execution::Token*>(observable);
      if ( !token->node && token->state == Execution::Token::State::CREATED ) {
        static_cast<Scenario&>(scenario).tokensAwaitingReadyEvent.push_back(token->weak_from_this());
      }
    }
  }

  void dispatchEvent(const Execution::SystemState* systemState, Execution::Scenario& executionScenario, Execution::EventQueue& queue) const override {
    auto& scenario = static_cast<Scenario&>(executionScenario);
    auto extensionElements = process->extensionElements->as<Model::ExtensionElements>();
    if ( !scenario.instantiated ) {
      scenario.instantiated = true;
      BPMNOS::Values status(extensionElements->attributes.size());
      status[Model::ExtensionElements::Index::Timestamp] = systemState->getTime();
      BPMNOS::Values data(extensionElements->data.size());
      data[Model::ExtensionElements::Index::Instance] = instanceId;
      queue.push_back(std::make_shared<Execution::InstantiationEvent>(process, status, data));
      return;
    }
    for ( auto& token_ptr : scenario.tokensAwaitingReadyEvent ) {
      if ( auto token = token_ptr.lock() ) {
        BPMNOS::Values status(extensionElements->attributes.size());
        BPMNOS::Values data(extensionElements->data.size());
        data[Model::ExtensionElements::Index::Instance] = instanceId;
        queue.push_back(std::make_shared<Execution::ReadyEvent>(token.get(), status, data));
      }
    }
    scenario.tokensAwaitingReadyEvent.clear();
    if ( !queue.empty() ) {
      return;
    }
    if ( !scenario.signalRaised && systemState->getTime() >= signalTime ) {
      scenario.signalRaised = true;
      queue.push_back(std::make_shared<Execution::SignalBroadcastEvent>(Execution::Signal(BPMNOS::to_number(std::string("Signal"),STRING), signalContent)));
      return;
    }
    if ( systemState->getTime() >= terminationTime ) {
      queue.push_back(std::make_shared<Execution::TerminationEvent>());
    }
  }

  BPMNOS::Values getGlobals([[maybe_unused]] const Execution::Scenario& scenario) const override {
    BPMNOS::Values globals(model->attributes.size());
    globals[Model::ExtensionElements::Index::Objective] = 0;
    return globals;
  }

  BPMNOS::number getEarliestInstantiationTime([[maybe_unused]] const Execution::Scenario& scenario) const override {
    return 0;
  }

  const std::shared_ptr<const Model::Model> model;
  const BPMN::Process* const process;
  const BPMNOS::number instanceId;
  const BPMNOS::number signalTime;
  const BPMNOS::VariedValueMap signalContent;
  const BPMNOS::number terminationTime;
};

SCENARIO( "Signal raised by the environment", "[data][provider][signal]" ) {
  const std::string modelFile = "tests/execution/signal/Signal.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A data provider instantiating the recipient only, so that nothing in the model throws the signal" ) {
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto process = model->processes.back().get();
    REQUIRE( process->id == "Process_2" );
    BPMNOS::VariedValueMap content;
    content.emplace( "Emitter", std::string("World") );
    auto dataProvider = std::make_shared<const ScriptedDataProvider>(model, process, "Instance_2", 2, content, 5);

    WHEN( "The environment raises the signal at time 2" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The signal broadcast event is recorded before the signal" ) {
        std::optional<size_t> eventIndex, signalIndex;
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          auto& entry = recorder.log[i];
          if ( !eventIndex && entry.contains("event") && entry["event"] == "signal" ) {
            eventIndex = i;
          }
          if ( !signalIndex && entry.contains("name") && entry["name"] == "Signal" ) {
            signalIndex = i;
          }
        }
        REQUIRE( eventIndex.has_value() );
        REQUIRE( signalIndex.has_value() );
        REQUIRE( eventIndex.value() < signalIndex.value() );
        REQUIRE( recorder.log[eventIndex.value()]["signal"]["content"]["Emitter"] == "World" );
      }

      AND_THEN( "The signal is recorded like one thrown within the model" ) {
        auto signalLog = recorder.find(nlohmann::json{{"name","Signal"}});
        REQUIRE( signalLog.size() == 1 );
        REQUIRE( signalLog.front()["content"]["Emitter"] == "World" );
      }

      AND_THEN( "The waiting token receives it at time 2" ) {
        auto recipientLog = recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state","COMPLETED"}});
        REQUIRE( recipientLog.size() == 1 );
        REQUIRE( recipientLog.front()["status"]["emitter"] == "World" );
        REQUIRE( recipientLog.front()["status"]["timestamp"] == 2.0 );
      }

      AND_THEN( "The run ends when the termination event is processed at time 5" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        REQUIRE( engine.getCurrentTime() == 5 );
      }
    }
  }
}
