#include "prelude.h"
#include <chrono>
#include <deque>

SCENARIO( "Clock ticks and termination supplied by the data provider", "[data][provider]" ) {
  auto model = std::make_shared<const Model::Model>("tests/execution/timer/Timer.bpmn");

  GIVEN( "A single instance whose timer is triggered at time 10" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger := 10\n"
    ;

    // the reference run, on a data provider without clock tick duration
    auto referenceDataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    Execution::Engine referenceEngine(model);
    Execution::InstantEntry referenceEntryHandler;
    Execution::InstantExit referenceExitHandler;
    referenceEntryHandler.connect(&referenceEngine);
    referenceExitHandler.connect(&referenceEngine);
    Execution::Recorder referenceRecorder;
    referenceRecorder.subscribe(&referenceEngine);
    referenceEngine.run(referenceDataProvider->createScenario());

    THEN( "The run on the data provider without clock tick duration ends exactly when the termination event is processed" ) {
      REQUIRE( referenceRecorder.log.back()["event"] == "termination" );
      auto terminationLog = referenceRecorder.find(nlohmann::json{{"event","termination"}});
      REQUIRE( terminationLog.size() == 1 );
      auto timerLog = referenceRecorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}});
      REQUIRE( timerLog.size() == 1 );
    }

    WHEN( "The engine runs on a data provider with a clock tick duration" ) {
      const std::chrono::milliseconds clockTickDuration{20};
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv, clockTickDuration);
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      auto scenario = dataProvider->createScenario();
      auto start = std::chrono::steady_clock::now();
      engine.run(std::move(scenario));
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);

      THEN( "Every decision of the controller is processed before the next clock tick, as without a clock tick duration" ) {
        REQUIRE( recorder.log == referenceRecorder.log );
      }
      THEN( "The clock ticks follow the wall clock" ) {
        // the opening clock tick is processed by the engine, every later one is due the given duration
        // after the previous one
        auto clockTicks = recorder.find(nlohmann::json{{"event","clocktick"}}).size();
        REQUIRE( clockTicks > 2 );
        REQUIRE( elapsed >= static_cast<std::chrono::milliseconds::rep>( clockTicks - 2 ) * clockTickDuration );
      }
    }

    WHEN( "The engine runs on a data provider with the end time 5" ) {
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      dataProvider->setEndTime(5);
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The run ends when the termination event is processed, with the instance still running" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        REQUIRE( engine.getCurrentTime() == 5 );
        REQUIRE( engine.getSystemState()->globalStateMachine->tokens.size() == 1 );
        auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}});
        REQUIRE( timerLog.empty() );
      }
    }
  }
}

/**
 * Dispatcher supplying a given number of clock ticks, one whenever it is asked, as a caller holding time
 * advances it.
 */
class ClockTicks : public Execution::EventDispatcher {
public:
  ClockTicks(unsigned int count)
    : remaining(count)
  {
  }

  std::shared_ptr<Execution::Event> dispatchEvent(const Execution::SystemState* systemState) override {
    if ( remaining == 0 ) {
      return nullptr;
    }
    remaining--;
    return std::make_shared<Execution::ClockTickEvent>(systemState);
  }

  unsigned int remaining;
};

SCENARIO( "Time held by the data provider", "[data][provider]" ) {
  auto model = std::make_shared<const Model::Model>("tests/execution/timer/Timer.bpmn");

  GIVEN( "A single instance whose timer is triggered at time 10 and a data provider never advancing time itself" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger := 10\n"
    ;
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv, std::chrono::milliseconds::max());

    WHEN( "The engine advances without a dispatcher supplying clock ticks" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.initialize(dataProvider->createScenario());
      for ( int round = 0; round < 20; round++ ) {
        REQUIRE( engine.advance() );
      }

      THEN( "Time stands still at the beginning of the run" ) {
        REQUIRE( engine.getCurrentTime() == 0 );
        // the opening clock tick is processed by the engine itself
        REQUIRE( recorder.find(nlohmann::json{{"event","clocktick"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}}).empty() );
      }
    }

    WHEN( "A dispatcher supplies ten clock ticks" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      ClockTicks clockTicks(10);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      clockTicks.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The clock ticks advance time to the timer, which is triggered" ) {
        auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}});
        REQUIRE( timerLog.size() == 1 );
        REQUIRE( timerLog.front()["status"]["timestamp"] == 10.0 );
      }
      THEN( "The run ends once nothing is left, although the data provider never advances time itself" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        REQUIRE( engine.getCurrentTime() == 10 );
      }
    }
  }
}

/**
 * Dispatcher supplying the events a caller has enqueued, in the order in which they were enqueued.
 */
class QueuedEvents : public Execution::EventDispatcher {
public:
  std::shared_ptr<Execution::Event> dispatchEvent([[maybe_unused]] const Execution::SystemState* systemState) override {
    if ( events.empty() ) {
      return nullptr;
    }
    auto event = events.front();
    events.pop_front();
    return event;
  }

  std::deque<std::shared_ptr<Execution::Event>> events;
};

SCENARIO( "A caller acting while the engine waits", "[data][provider]" ) {
  auto model = std::make_shared<const Model::Model>("tests/execution/timer/Timer.bpmn");

  GIVEN( "A single instance whose timer is triggered at time 10 and a data provider never advancing time itself" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger := 10\n"
    ;
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv, std::chrono::milliseconds::max());
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    QueuedEvents queuedEvents;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    queuedEvents.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);

    // the caller advances time by one clock tick whenever the engine waits
    unsigned int waits = 0;
    auto tick = [&]() {
      waits++;
      queuedEvents.events.push_back(std::make_shared<Execution::ClockTickEvent>(engine.getSystemState()));
    };

    WHEN( "The caller enqueues a clock tick whenever the engine waits" ) {
      engine.wait = tick;
      engine.run(dataProvider->createScenario());

      THEN( "The engine waits instead of sleeping, and every clock tick enqueued is processed in the following round" ) {
        REQUIRE( waits == 10 );
        auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}});
        REQUIRE( timerLog.size() == 1 );
        REQUIRE( timerLog.front()["status"]["timestamp"] == 10.0 );
      }
      THEN( "The run ends once nothing is left" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        REQUIRE( engine.getCurrentTime() == 10 );
      }
    }

    WHEN( "The caller enqueues a termination event the first time the engine waits" ) {
      engine.wait = [&]() {
        waits++;
        queuedEvents.events.push_back(std::make_shared<Execution::TerminationEvent>());
      };
      engine.run(dataProvider->createScenario());

      THEN( "The run ends with the instance still running" ) {
        REQUIRE( waits == 1 );
        REQUIRE( engine.getCurrentTime() == 0 );
        REQUIRE( engine.getSystemState()->globalStateMachine->tokens.size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}}).empty() );
      }

      AND_WHEN( "The run is resumed with the caller enqueuing clock ticks" ) {
        waits = 0;
        engine.wait = tick;
        engine.resume();

        THEN( "The run continues from where it ended until nothing is left" ) {
          REQUIRE( waits == 10 );
          auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}});
          REQUIRE( timerLog.size() == 1 );
          REQUIRE( timerLog.front()["status"]["timestamp"] == 10.0 );
          REQUIRE( recorder.find(nlohmann::json{{"event","termination"}}).size() == 2 );
          REQUIRE( engine.getCurrentTime() == 10 );
        }
      }
    }
  }
}

SCENARIO( "No clock tick before the first request to the controller after installing a system state", "[data][provider]" ) {
  auto model = std::make_shared<const Model::Model>("tests/execution/task/Task_with_linear_expression.bpmn");

  GIVEN( "A system state in which a token awaits the decision to enter a task" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    // without an entry handler the token awaits the entry decision, and the run ends at time 0
    auto sourceDataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    sourceDataProvider->setEndTime(0);
    Execution::Engine sourceEngine(model);
    sourceEngine.run(sourceDataProvider->createScenario());
    REQUIRE_FALSE( sourceEngine.getSystemState()->pendingEntryDecisions.empty() );

    WHEN( "The system state is installed in an engine and the engine advances" ) {
      auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.initializeSystemState(dataProvider->createScenario(), sourceEngine.getSystemState());
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
    : DataProvider(model)
    , model(std::move(model))
    , process(process)
    , instanceId(BPMNOS::to_number(instanceId,STRING))
    , signalTime(signalTime)
    , signalContent(std::move(signalContent))
    , terminationTime(terminationTime)
  {
  }

  std::unique_ptr<Execution::Scenario> createScenario([[maybe_unused]] unsigned int realisation = 0) const override {
    return std::make_unique<Scenario>(std::static_pointer_cast<const ScriptedDataProvider>(shared_from_this()));
  }

  void notice(const Execution::Observable* observable, Execution::Scenario& scenario, [[maybe_unused]] Execution::EventQueue& queue) const override {
    if ( observable->getObservableType() == Execution::Observable::Type::Token ) {
      auto token = static_cast<const Execution::Token*>(observable);
      if ( token->node->represents<BPMN::Process>() && token->state == Execution::Token::State::CREATED ) {
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
    return BPMNOS::Values(model->attributes.size());
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
