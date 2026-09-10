class Simulator; // Used to simulate the obserable world

/**
 * The world model feeds an ObservedScenario with what it observes in the world do. The world model also advances time.
 *
 * The implementation below uses a Simulator based on a stochastic scenario in a dedicated engine.
 * The world model observes this simulation to update the ObservedScenario.
 */
class WorldModel : public Execution::EventDispatcher {
public:
  WorldModel(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instances, unsigned int seed);
  ~WorldModel();

  /// The scenario an engine is run on.
  Model::ObservedScenario& getScenario() { return observed; }

  /// The world being observed, so that the test can watch it too.
  Simulator& getSimulator() { return *simulator; }

  /// Set the world running and report what it holds at the given time, before any engine asks for it.
  void start(BPMNOS::number time);

  /// Releases a tick to the engine only once the world has reached the time it advances to, so the run
  /// can never get ahead of the world it observes.
  std::shared_ptr<Execution::Event> dispatchEvent( const Execution::SystemState* systemState ) override;

private:
  /// Let the world reach the given time, and report everything it has been seen to do by then. An
  /// instantiation is reported before anything else about that instance, which is all the ordering the
  /// scenario requires.
  void report(BPMNOS::number time);

  std::unique_ptr<Simulator> simulator;
  std::unique_ptr<Model::Model> model;   ///< declared before the scenario, which points at it
  Model::ObservedScenario observed;
};

/// What a world has been seen to do since it was last asked.
struct Observations {
  struct Instantiation { const BPMN::Process* process; BPMNOS::number instanceId; BPMNOS::number time; };
  struct Value { BPMNOS::number instanceId; const Model::Attribute* attribute; std::optional<BPMNOS::number> value; };
  struct Status { BPMNOS::number instanceId; const BPMN::Node* node; BPMNOS::Values status; };

  std::vector<Instantiation> instantiations;
  std::vector<Value> values;
  std::vector<Status> readyStatuses;
  std::vector<Status> completionStatuses;
};

/// A simulation based on a stochastic scenario running in a dedicated engine.
class Simulator : public Execution::Observer {
public:
  Simulator(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instances, unsigned int seed)
    : provider(modelFile, folders, instances, seed)
    , scenario(provider.createScenario(0))
  {
    controller.connect(&engine);
    timeHandler.connect(&engine);
    // subscribed before the world is set running, since a status noticed by nobody is a status nothing
    // can report
    engine.addSubscriber(this, Execution::Observable::Type::Token);
  }

  /// Set the world running at the given time.
  void start(BPMNOS::number time) { engine.initialize(scenario.get(), time); }

  /// Let the world reach the given time.
  void advanceTo(BPMNOS::number time) { engine.resume(time); }

  /// Everything the world has been seen to do up to the given time, and not yet reported.
  Observations take(BPMNOS::number time) {
    Observations observations;

    for ( auto instance : scenario->getCreatedInstances(time) ) {
      observations.instantiations.push_back({instance->process, (BPMNOS::number)instance->id, instance->instantiationTime});

      for ( auto node : nodesWithExtensionElements(instance->process) ) {
        auto extensionElements = node->extensionElements->as<const Model::ExtensionElements>();

        // Values are taken by node rather than by attribute, because a node's values become knowable
        // together: the world answers for a whole node or not at all, and an attribute of a node it
        // answers for that has no value is seen to have none, which is not the same as not being seen.
        if ( auto status = scenario->getStatus((BPMNOS::number)instance->id, node, time); status.has_value() ) {
          for ( size_t i = 0; i < extensionElements->attributes.size(); i++ ) {
            observations.values.push_back({(BPMNOS::number)instance->id, extensionElements->attributes[i].get(), status->at(i)});
          }
        }
        if ( auto data = scenario->getData((BPMNOS::number)instance->id, node, time); data.has_value() ) {
          for ( size_t i = 0; i < extensionElements->data.size(); i++ ) {
            observations.values.push_back({(BPMNOS::number)instance->id, extensionElements->data[i].get(), data->at(i)});
          }
        }
      }
    }

    observations.readyStatuses = std::move(noticedReady);
    observations.completionStatuses = std::move(noticedCompletion);
    noticedReady.clear();
    noticedCompletion.clear();
    return observations;
  }

  Execution::Engine& getEngine() { return engine; }

  /// Take the status a token carries at the moment the world determines it.
  void notice(const Execution::Observable* observable) override {
    if ( observable->getObservableType() != Execution::Observable::Type::Token ) {
      return;
    }
    auto token = static_cast<const Execution::Token*>(observable);
    if ( !token->node || !token->node->represents<BPMN::Activity>() ) {
      return;
    }
    if ( token->state == Execution::Token::State::READY ) {
      noticedReady.push_back({token->getInstanceId(), token->node, token->status});
    }
    else if ( token->state == Execution::Token::State::COMPLETED && token->node->represents<BPMN::Task>() ) {
      noticedCompletion.push_back({token->getInstanceId(), token->node, token->status});
    }
  }

private:
  static std::vector<BPMN::Node*> nodesWithExtensionElements(const BPMN::Process* process) {
    return const_cast<BPMN::Process*>(process)->find_all(
      [](BPMN::Node* node) {
        return node->extensionElements && node->extensionElements->represents<Model::ExtensionElements>();
      }
    );
  }

  std::vector<Observations::Status> noticedReady;
  std::vector<Observations::Status> noticedCompletion;

  Model::StochasticDataProvider provider;
  std::unique_ptr<Model::Scenario> scenario;   ///< declared before the engine, so it outlives it
  Execution::Engine engine;
  Execution::GreedyController controller{std::make_shared<Execution::LocalEvaluator>()};
  Execution::TimeWarp timeHandler;
};

inline WorldModel::WorldModel(const std::string& modelFile, const std::vector<std::string>& folders, const std::string& instances, unsigned int seed)
  : simulator(std::make_unique<Simulator>(modelFile, folders, instances, seed))
  , model(std::make_unique<Model::Model>(modelFile, folders))
  , observed(model.get(), {})
{
}

/// Defined here rather than defaulted in the class, since destroying the simulator needs its definition.
inline WorldModel::~WorldModel() = default;

inline void WorldModel::start(BPMNOS::number time) {
  simulator->start(time);
  report(time);
}

inline std::shared_ptr<Execution::Event> WorldModel::dispatchEvent( const Execution::SystemState* systemState ) {
  report( systemState->getTime() + Execution::ClockTickEvent::clockTick );
  return std::make_shared<Execution::ClockTickEvent>(systemState);
}

inline void WorldModel::report(BPMNOS::number time) {
  simulator->advanceTo(time);
  auto observations = simulator->take(time);

  for ( auto& [process, instanceId, instantiationTime] : observations.instantiations ) {
    observed.observeInstantiation(process, instanceId, instantiationTime);
  }
  for ( auto& [instanceId, attribute, value] : observations.values ) {
    observed.observeValue(instanceId, attribute, value);
  }
  for ( auto& [instanceId, node, status] : observations.readyStatuses ) {
    observed.observeReadyStatus(instanceId, node, status);
  }
  for ( auto& [instanceId, node, status] : observations.completionStatuses ) {
    observed.observeCompletionStatus(instanceId, node, status);
  }
}

SCENARIO( "An observed scenario fed from a simulated world", "[data][observed]" ) {
  const std::string modelFile = "tests/data/stochastic/Executable_process.bpmn";

  GIVEN( "A world model reporting what a simulator produces" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := 0;;;\n"
      "Instance_1; Process_1; x := 1;;;\n"
      "Instance_1; Activity_1; data := 5;;;\n"
      "Instance_1; Activity_1; y := 2;;;\n"
      "Instance_1; Task_2; z := 3;;; timestamp := timestamp + triangular(2,2,2)\n"
    ;

    WorldModel world(modelFile, {}, csv, 42);

    WHEN( "an engine runs on what the world reports" ) {
      Execution::Recorder simulatorRecorder;
      simulatorRecorder.subscribe(&world.getSimulator().getEngine());

      world.start(0);

      Execution::Engine engine;
      Execution::GreedyController controller(std::make_shared<Execution::LocalEvaluator>());
      controller.connect(&engine);
      world.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);

      // An observed scenario never reports itself complete, so the run is bounded from outside
      engine.run(&world.getScenario(), 0, 20);

      THEN( "the observing run reaches the same states as the simulated world" ) {
        auto simulated = simulatorRecorder.find(nlohmann::json{{"nodeId","Task_2"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
        auto observed = recorder.find(nlohmann::json{{"nodeId","Task_2"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});

        REQUIRE( !simulated.empty() );
        REQUIRE( !observed.empty() );
        REQUIRE( simulated.back()["state"] == "DEPARTED" );
        REQUIRE( observed.back()["state"] == "DEPARTED" );
        REQUIRE( simulated.back()["timestamp"] == observed.back()["timestamp"] );
      }

      THEN( "the observing run completes the process at the same time as the simulated world" ) {
        auto simulated = simulatorRecorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        auto observed = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});

        REQUIRE( !simulated.empty() );
        REQUIRE( !observed.empty() );
        REQUIRE( simulated.back()["state"] == "DONE" );
        REQUIRE( observed.back()["state"] == "DONE" );
        REQUIRE( simulated.back()["timestamp"] == observed.back()["timestamp"] );
      }
    }

    WHEN( "the observed scenario is asked to be copied" ) {
      THEN( "it refuses" ) {
        REQUIRE_THROWS_AS( world.getScenario().clone(1, 0), std::logic_error );
      }
    }

    WHEN( "nothing has been observed" ) {
      THEN( "no instance is known and the scenario is never complete" ) {
        REQUIRE( world.getScenario().getInstances(100).empty() );
        REQUIRE( world.getScenario().getCreatedInstances(100).empty() );
        REQUIRE( !world.getScenario().isCompleted(100) );
        REQUIRE( world.getScenario().getEarliestInstantiationTime() == std::numeric_limits<BPMNOS::number>::max() );
      }
    }
  }
}
