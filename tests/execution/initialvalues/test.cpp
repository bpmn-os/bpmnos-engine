#include "prelude.h"
#include <set>
#include <unordered_map>

namespace {

const Model::AttributeRegistry& registryOf(const BPMN::Node* node) {
  return node->extensionElements->as<Model::ExtensionElements>()->attributeRegistry;
}

} // namespace

SCENARIO( "Values of a scope instance built in declaration order", "[execution][initialvalues]" ) {
  const std::string modelFile = "tests/execution/initialvalues/Declaration_order.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance giving a value to an attribute the model does not initialise" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; later := 4\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto process = model->processes.front().get();
    auto& registry = registryOf(process);

    WHEN( "The values are built directly" ) {
      auto scenario = dataProvider->createScenario();
      auto globals = dataProvider->getGlobals(*scenario);
      BPMNOS::SharedData inherited(globals);
      std::unordered_map<const Model::Attribute*, BPMNOS::number> given{ { registry["later"], 4 } };
      auto instance = BPMNOS::number(stringRegistry("Instance_1"));
      Execution::InstanceValues values(*dataProvider, process, 5, BPMNOS::Status{}, inherited, instance, given, {});
      BPMNOS::SharedData data(inherited, values.data);

      THEN( "The timestamp is the time at which the instance comes into being" ) {
        REQUIRE( registry.getValue(registry["timestamp"], values.status, data) == 5 );
        REQUIRE( registry.getValue(registry["start"], values.status, data) == 5 );
      }
      THEN( "A given value is taken and an initial expression reads the values declared before it" ) {
        REQUIRE( registry.getValue(registry["later"], values.status, data) == 4 );
        REQUIRE( registry.getValue(registry["second"], values.status, data) == 5 );
        REQUIRE( registry.getValue(registry["doubled"], values.status, data) == 10 );
      }
      THEN( "The instance is given its identity before the walk" ) {
        REQUIRE( registry.getValue(registry["self"], values.status, data) == instance );
      }
      THEN( "A further value is assigned over the values built" ) {
        Model::Expression expression(InputEncoder::fragment("second := later * 3"), registry);
        values.assign(expression);
        REQUIRE( registry.getValue(registry["second"], values.status, data) == 12 );
      }
    }

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The instantiation gives the instance its identity and timestamp only" ) {
        auto createdLog = recorder.find(nlohmann::json{{"processId","Process_1"},{"state","CREATED"}}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        REQUIRE( createdLog.size() == 1 );
        REQUIRE( createdLog[0]["instanceId"] == "Instance_1" );
        REQUIRE( createdLog[0]["status"]["later"].is_null() );
        REQUIRE( createdLog[0]["status"]["second"].is_null() );
      }
      THEN( "The ready event gives the instance its values" ) {
        auto enteredLog = recorder.find(nlohmann::json{{"processId","Process_1"},{"state","ENTERED"}}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        REQUIRE( enteredLog.size() == 1 );
        REQUIRE( enteredLog[0]["status"]["later"] == 4 );
        REQUIRE( enteredLog[0]["status"]["second"] == 5 );
        REQUIRE( enteredLog[0]["data"]["doubled"] == 10 );
        REQUIRE( enteredLog[0]["data"]["self"] == "Instance_1" );
      }
    }
  }
}

SCENARIO( "Ready values over the initial values", "[execution][initialvalues]" ) {
  const std::string modelFile = "tests/execution/initialvalues/Ready_values.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  auto model = std::make_shared<const Model::Model>(modelFile);
  const std::string header = "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n";
  const std::string process = "Instance_1; Process_1; timestamp := 0; ; ;\n";

  auto run = [&](const std::string& csv) {
    auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(model, csv);
    Execution::Engine engine(model);
    Execution::InstantEntry entryHandler;
    Execution::InstantExit exitHandler;
    entryHandler.connect(&engine);
    exitHandler.connect(&engine);
    Execution::Recorder recorder;
    recorder.subscribe(&engine);
    engine.run(dataProvider->createScenario());
    return recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","READY"}});
  };

  GIVEN( "Ready expressions assigning a model-initialised status attribute and an owned data attribute" ) {
    auto readyLog = run(header + process +
      "Instance_1; Activity_1; ; ; duration := 7;\n"
      "Instance_1; Activity_1; ; ; budget := 3;\n"
    );
    THEN( "The ready values overwrite the initial values" ) {
      REQUIRE( readyLog.size() == 1 );
      REQUIRE( readyLog[0]["status"]["duration"] == 7 );
      REQUIRE( readyLog[0]["status"]["ready_at"] == 0 );
      REQUIRE( readyLog[0]["data"]["budget"] == 3 );
    }
  }

  GIVEN( "A ready expression assigning the timestamp, followed by another" ) {
    auto readyLog = run(header + process +
      "Instance_1; Activity_1; ; ; timestamp := timestamp + 3;\n"
      "Instance_1; Activity_1; ; ; duration := 7;\n"
    );
    THEN( "The timestamp fixes when the token becomes ready, and the initial values are built then" ) {
      REQUIRE( readyLog.size() == 1 );
      REQUIRE( readyLog[0]["status"]["timestamp"] == 3 );
      REQUIRE( readyLog[0]["status"]["ready_at"] == 3 );
      REQUIRE( readyLog[0]["status"]["duration"] == 7 );
    }
  }

  GIVEN( "Ready expressions violating the rules" ) {
    THEN( "A ready expression assigning the timestamp after another is refused" ) {
      REQUIRE_THROWS_WITH( Execution::StochasticDataProvider(model, header + process +
        "Instance_1; Activity_1; ; ; duration := 7;\n"
        "Instance_1; Activity_1; ; ; timestamp := timestamp + 3;\n"
      ), Catch::Matchers::ContainsSubstring("assigning the timestamp must be the first ready expression") );
    }
    THEN( "A ready expression assigning the timestamp from an attribute of the scope is refused" ) {
      REQUIRE_THROWS_WITH( Execution::StochasticDataProvider(model, header + process +
        "Instance_1; Activity_1; ; ; timestamp := duration + 3;\n"
      ), Catch::Matchers::ContainsSubstring("must read only what the scope inherits") );
    }
    THEN( "A ready expression assigning inherited data is refused" ) {
      REQUIRE_THROWS_WITH( Execution::StochasticDataProvider(model, header + process +
        "Instance_1; Activity_1; ; ; total := 1;\n"
      ), Catch::Matchers::ContainsSubstring("must assign a status attribute or a data attribute the scope owns") );
    }
  }
}

SCENARIO( "Event subprocess initialised when it is created", "[execution][initialvalues]" ) {
  const std::string modelFile = "tests/execution/initialvalues/Event_subprocess_created.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A process running for 12 time units with a non-interrupting timer event subprocess" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      auto startLog = recorder.find(nlohmann::json{{"nodeId","TimerStartEvent_1"},{"state","COMPLETED"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});

      THEN( "Each instance keeps the values it was created with when it is triggered" ) {
        // instances are created at 0, 5 and 10; the first two are triggered at 5 and 10, the third is discarded
        REQUIRE( startLog.size() == 2 );
        REQUIRE( startLog[0]["status"]["timestamp"] == 5 );
        REQUIRE( startLog[0]["status"]["created"] == 0 );
        REQUIRE( startLog[1]["status"]["timestamp"] == 10 );
        REQUIRE( startLog[1]["status"]["created"] == 5 );
      }
      THEN( "Each instance reads its own identity" ) {
        REQUIRE( startLog.size() == 2 );
        REQUIRE( startLog[0]["status"]["self"] == startLog[0]["instanceId"] );
        REQUIRE( startLog[1]["status"]["self"] == startLog[1]["instanceId"] );
        REQUIRE( startLog[0]["status"]["self"] != startLog[1]["status"]["self"] );
        REQUIRE( startLog[0]["status"]["self"] != "Instance_1" );
      }
      THEN( "The initial weighted data count for every instance created, those set by the start event for every trigger" ) {
        REQUIRE( engine.getSystemState()->getObjective() == 3 + 2 );
      }
    }
  }
}

SCENARIO( "Instance started by a trigger initialised before the content is written", "[execution][initialvalues]" ) {
  const std::string modelFile = "tests/execution/initialvalues/Triggered_instance.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "An instance sending the message that starts another" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The content overwrites an initial value and the operators of the start event see both" ) {
        auto startLog = recorder.find(nlohmann::json{{"nodeId","MessageStartEvent"},{"state","DEPARTED"}});
        REQUIRE( startLog.size() == 1 );
        REQUIRE( startLog[0]["status"]["received"] == 5 );
        REQUIRE( startLog[0]["status"]["initial"] == 7 );
        REQUIRE( startLog[0]["status"]["seen"] == 12 );
      }
    }
  }
}

SCENARIO( "Start event operators per instance of a multi-instance subprocess", "[execution][initialvalues]" ) {
  const std::string modelFile = "tests/execution/initialvalues/Multi-instance_start_event.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A multi-instance subprocess with two instances" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The operators of the start event read the index of each instance" ) {
        auto startLog = recorder.find(nlohmann::json{{"nodeId","StartEvent_2"},{"state","DEPARTED"}});
        REQUIRE( startLog.size() == 2 );
        std::set<int> seen{ startLog[0]["status"]["seen"].get<int>(), startLog[1]["status"]["seen"].get<int>() };
        REQUIRE( seen == std::set<int>{ 1, 2 } );
        REQUIRE( startLog[0]["instanceId"] != startLog[1]["instanceId"] );
      }
    }
  }
}
