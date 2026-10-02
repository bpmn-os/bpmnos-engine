SCENARIO( "Two processes with signal", "[execution][signal]" ) {
  const std::string modelFile = "tests/execution/signal/Signal.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started with one emitter before one recipient" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 0\n"
        "Instance_2; Process_2; timestamp := 1\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(), 0, 2);
      THEN( "The signal is emitted before it can be received" ) {
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( recipientLog.size() == 0 );
      }
      AND_THEN( "The signal is recorded although nobody receives it" ) {
        auto signalLog = recorder.find(nlohmann::json{{"name","Signal"}});
        REQUIRE( signalLog.size() == 1 );
        REQUIRE( !signalLog.front()["content"]["Emitter"].is_null() );
      }
    }

    WHEN( "The engine is started with one emitter after one recipient" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 1\n"
        "Instance_2; Process_2; timestamp := 0\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The signal is received" ) {
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( recipientLog.size() == 1 );
        REQUIRE( recipientLog.front()["status"]["emitter"] == "Instance_1" );
      }
      AND_THEN( "The signal is recorded once, with the content it carries" ) {
        auto signalLog = recorder.find(nlohmann::json{{"name","Signal"}});
        REQUIRE( signalLog.size() == 1 );
        REQUIRE( !signalLog.front()["content"]["Emitter"].is_null() );
      }
    }

    WHEN( "The engine is started with one emitter after two recipients" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 1\n"
        "Instance_2; Process_2; timestamp := 0\n"
        "Instance_3; Process_2; timestamp := 0\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The signal is received by both recipients" ) {
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( recipientLog.size() == 2 );
        REQUIRE( recipientLog.front()["status"]["emitter"] == "Instance_1" );
        REQUIRE( recipientLog.back()["status"]["emitter"] == "Instance_1" );
      }
      AND_THEN( "One signal is recorded, however many receive it" ) {
        REQUIRE( recorder.find(nlohmann::json{{"name","Signal"}}).size() == 1 );
      }
    }

    WHEN( "The engine is started with two emitters after two recipients" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_0; Process_1; timestamp := 1\n"
        "Instance_1; Process_1; timestamp := 1\n"
        "Instance_2; Process_2; timestamp := 0\n"
        "Instance_3; Process_2; timestamp := 0\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "The one of the signals is received by both recipients" ) {
        auto recipientLog =recorder.find(nlohmann::json{{"nodeId","SignalEvent_2"},{"state", "COMPLETED"}});
        REQUIRE( recipientLog.size() == 2 );
        REQUIRE( recipientLog.front()["status"]["emitter"] == recipientLog.back()["status"]["emitter"] );
      }
      AND_THEN( "Each emitter records a signal of its own, the second reaching nobody" ) {
        auto signalLog = recorder.find(nlohmann::json{{"name","Signal"}});
        REQUIRE( signalLog.size() == 2 );
        REQUIRE( signalLog.front()["content"]["Emitter"] != signalLog.back()["content"]["Emitter"] );
      }
    }

  }
}

SCENARIO( "Signal raised by the environment", "[execution][signal]" ) {
  const std::string modelFile = "tests/execution/signal/Signal.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An observed recipient and no emitter" ) {
    // only Process_2 is instantiated, so nothing in the model ever throws the signal
    Model::Model model(modelFile);
    Model::ObservedScenario scenario(&model, {});
    auto process = model.processes.back().get();
    REQUIRE( process->id == "Process_2" );
    auto instanceId = BPMNOS::to_number(std::string("Instance_2"),STRING);
    auto signalName = BPMNOS::to_number(std::string("Signal"),STRING);

    // the values of the process are observed together with the instantiation, the data of an observed
    // instance being disclosed only once every data attribute has been observed
    auto extensionElements = process->extensionElements->as<Model::ExtensionElements>();
    scenario.observeInstantiation(process, instanceId, 0);
    for ( auto& attribute : extensionElements->data ) {
      bool isInstance = ( attribute.get() == extensionElements->data[Model::ExtensionElements::Index::Instance].get() );
      scenario.observeValue(instanceId, attribute.get(), isInstance ? std::optional<BPMNOS::number>(instanceId) : std::nullopt);
    }
    scenario.observeValue(instanceId, extensionElements->attributes[Model::ExtensionElements::Index::Timestamp].get(), BPMNOS::number(0));

    WHEN( "The environment raises the signal at time 2" ) {
      BPMNOS::VariedValueMap content;
      content.emplace( "Emitter", std::string("World") );
      scenario.observeSignal(signalName, content, 2);

      Execution::Engine engine;
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      // an observed scenario never reports itself complete, so the run is bounded
      engine.run(&scenario, 0, 5);

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
    }
  }
}
