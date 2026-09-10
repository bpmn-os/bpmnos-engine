/**
 * The world here is a stochastic scenario driven by its own engine. A second engine runs on an
 * ObservedScenario that is told only what the world has already produced, and the two runs are compared.
 * Because the world knows what it is going to do and the observing run does not, any divergence is a
 * defect in how observations are reported or answered.
 *
 * The World below is also the smallest complete example of feeding an ObservedScenario: it shows what has
 * to be reported, and when it has to be reported for the engine to be able to proceed.
 */

/// Reports what a stochastic world has produced to an observed scenario, and paces an engine to it.
///
/// It watches the world's engine rather than interrogating the world's scenario, because a status is
/// answered and discarded within the step that produces it: by the time the world has advanced to a given
/// time, nothing is left to ask it about. What a token carries as it reaches READY or COMPLETED is exactly
/// the status that was determined for it, so the notification is both the earliest and the only reliable
/// moment to take it.
class World : public Execution::EventDispatcher, public Execution::Observer {
public:
  World(Execution::Engine& source, const Model::Scenario* truth, Model::ObservedScenario& observed)
    : source(source), truth(truth), observed(observed)
  {
    source.addSubscriber(this, Execution::Observable::Type::Token);
  }

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
      pendingReady.push_back({token->getInstanceId(), token->node, token->status});
    }
    else if ( token->state == Execution::Token::State::COMPLETED && token->node->represents<BPMN::Task>() ) {
      pendingCompletion.push_back({token->getInstanceId(), token->node, token->status});
    }
  }

  /// Advance the world to the given time and report everything it has produced by then.
  ///
  /// Instances and attribute values are read from the world's scenario, which retains them. Statuses come
  /// from what was noticed while the world advanced, since the scenario no longer holds them.
  ///
  /// Values are reported by node rather than by attribute, because a node's values become knowable
  /// together: the world answers for a whole node or not at all, and an attribute of a disclosed node that
  /// has no value is observed to have none, which is not the same as not having been observed.
  void report(BPMNOS::number time) {
    source.resume(time);

    for ( auto instance : truth->getCreatedInstances(time) ) {
      observed.observeInstantiation(instance->process, (BPMNOS::number)instance->id, instance->instantiationTime);

      for ( auto node : nodesWithExtensionElements(instance->process) ) {
        auto extensionElements = node->extensionElements->as<const Model::ExtensionElements>();

        if ( auto status = truth->getStatus((BPMNOS::number)instance->id, node, time); status.has_value() ) {
          for ( size_t i = 0; i < extensionElements->attributes.size(); i++ ) {
            observed.observeValue((BPMNOS::number)instance->id, extensionElements->attributes[i].get(), status->at(i));
          }
        }

        if ( auto data = truth->getData((BPMNOS::number)instance->id, node, time); data.has_value() ) {
          for ( size_t i = 0; i < extensionElements->data.size(); i++ ) {
            observed.observeValue((BPMNOS::number)instance->id, extensionElements->data[i].get(), data->at(i));
          }
        }
      }
    }

    for ( auto& [instanceId, node, status] : pendingReady ) {
      observed.observeReadyStatus(instanceId, node, status);
    }
    pendingReady.clear();

    for ( auto& [instanceId, node, status] : pendingCompletion ) {
      observed.observeCompletionStatus(instanceId, node, status);
    }
    pendingCompletion.clear();
  }

  /// Releases a tick to the observing engine only once the world has reached the time it advances to, so
  /// the observing run can never get ahead of the world it observes.
  std::shared_ptr<Execution::Event> dispatchEvent( const Execution::SystemState* systemState ) override {
    report( systemState->getTime() + Execution::ClockTickEvent::clockTick );
    return std::make_shared<Execution::ClockTickEvent>(systemState);
  }

private:
  static std::vector<BPMN::Node*> nodesWithExtensionElements(const BPMN::Process* process) {
    auto nodes = const_cast<BPMN::Process*>(process)->find_all(
      [](BPMN::Node* node) {
        return node->extensionElements && node->extensionElements->represents<Model::ExtensionElements>();
      }
    );
    return nodes;
  }

  struct Noticed {
    BPMNOS::number instanceId;
    const BPMN::Node* node;
    BPMNOS::Values status;
  };
  std::vector<Noticed> pendingReady;      ///< Noticed while the world advanced, not yet reported.
  std::vector<Noticed> pendingCompletion;

  Execution::Engine& source;
  const Model::Scenario* truth;
  Model::ObservedScenario& observed;
};

SCENARIO( "An observed scenario fed from a simulated world", "[data][observed]" ) {
  const std::string modelFile = "tests/data/stochastic/Executable_process.bpmn";

  GIVEN( "A stochastic world and an observed scenario of the same model" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1; timestamp := 0;;;\n"
      "Instance_1; Process_1; x := 1;;;\n"
      "Instance_1; Activity_1; data := 5;;;\n"
      "Instance_1; Activity_1; y := 2;;;\n"
      "Instance_1; Task_2; z := 3;;; timestamp := timestamp + triangular(2,2,2)\n"
    ;

    Model::StochasticDataProvider dataProvider(modelFile, csv, 42);
    auto truth = dataProvider.createScenario(0);

    Model::ObservedScenario observed(&dataProvider.getModel(), {});

    WHEN( "the world is run and observed, and a second engine runs on the observations" ) {
      // The world, run to completion by its own engine under a greedy policy
      Execution::Engine sourceEngine;
      Execution::GreedyController sourceController(std::make_shared<Execution::LocalEvaluator>());
      Execution::TimeWarp sourceTimeHandler;
      sourceController.connect(&sourceEngine);
      sourceTimeHandler.connect(&sourceEngine);
      Execution::Recorder sourceRecorder;
      sourceRecorder.subscribe(&sourceEngine);

      // Subscribed before the world is set running, since initialize already advances tokens and a status
      // noticed by nobody is a status nothing can report
      World world(sourceEngine, truth.get(), observed);

      sourceEngine.initialize(truth.get(), 0);
      world.report(0);   // the opening tick is issued by initialize, not by the dispatcher

      Execution::Engine observingEngine;
      Execution::GreedyController observingController(std::make_shared<Execution::LocalEvaluator>());
      observingController.connect(&observingEngine);
      world.connect(&observingEngine);
      Execution::Recorder observingRecorder;
      observingRecorder.subscribe(&observingEngine);

      // An observed scenario never reports itself complete, so the run is bounded from outside
      observingEngine.run(&observed, 0, 20);

      THEN( "the observing run reaches the same states as the world" ) {
        auto sourceLog = sourceRecorder.find(nlohmann::json{{"nodeId","Task_2"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
        auto observingLog = observingRecorder.find(nlohmann::json{{"nodeId","Task_2"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});

        REQUIRE( !sourceLog.empty() );
        REQUIRE( !observingLog.empty() );
        REQUIRE( sourceLog.back()["state"] == "DEPARTED" );
        REQUIRE( observingLog.back()["state"] == "DEPARTED" );
        REQUIRE( sourceLog.back()["timestamp"] == observingLog.back()["timestamp"] );
      }

      THEN( "the observing run completes the process at the same time as the world" ) {
        auto sourceLog = sourceRecorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        auto observingLog = observingRecorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});

        REQUIRE( !sourceLog.empty() );
        REQUIRE( !observingLog.empty() );
        REQUIRE( sourceLog.back()["state"] == "DONE" );
        REQUIRE( observingLog.back()["state"] == "DONE" );
        REQUIRE( sourceLog.back()["timestamp"] == observingLog.back()["timestamp"] );
      }
    }

    WHEN( "the observed scenario is asked to be copied" ) {
      THEN( "it refuses" ) {
        REQUIRE_THROWS_AS( observed.clone(1, 0), std::logic_error );
      }
    }

    WHEN( "nothing has been observed" ) {
      THEN( "no instance is known and the scenario is never complete" ) {
        REQUIRE( observed.getInstances(100).empty() );
        REQUIRE( observed.getCreatedInstances(100).empty() );
        REQUIRE( !observed.isCompleted(100) );
        REQUIRE( observed.getEarliestInstantiationTime() == std::numeric_limits<BPMNOS::number>::max() );
      }
    }
  }
}
