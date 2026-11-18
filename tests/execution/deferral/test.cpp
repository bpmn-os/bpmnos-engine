#include "prelude.h"
#include <fstream>
#include <sstream>

namespace {

/// Returns the model of an ad-hoc subprocess with two tasks, each incrementing the data attribute `x` of the
/// ad-hoc subprocess, whose performer states the given deferral, none if it is empty.
std::shared_ptr<const Model::Model> modelWith(const std::string& deferral) {
  std::ifstream file("tests/execution/adhocsubprocess/AdHocSubProcess.bpmn");
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string xml = buffer.str();
  const std::string performer = R"(<bpmn2:performer name="Sequential" />)";
  auto position = xml.find(performer);
  REQUIRE( position != std::string::npos );
  xml.replace(position, performer.size(), R"(<bpmn2:performer name="Sequential"><bpmn2:extensionElements>)" + deferral + R"(</bpmn2:extensionElements></bpmn2:performer>)");
  auto root = XML::XMLObject::createFromString(xml);
  REQUIRE( root );
  return std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});
}

/// An engine without controller, run until both tasks await entry at the performer, and deferrals subscribed to it.
struct Run {
  std::shared_ptr<Execution::StaticDataProvider> dataProvider;
  Execution::Deferrals deferrals; ///< Declared before the engine, so that it outlives the engine it is subscribed to
  Execution::Engine engine;

  Run(std::shared_ptr<const Model::Model> model)
    : dataProvider(std::make_shared<Execution::StaticDataProvider>(model,
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; timestamp := 0\n"
      ))
    , engine(model)
  {
    deferrals.connect(&engine);
    engine.initialize(dataProvider->createScenario(), 0);
    while ( engine.getSystemState()->pendingEntryDecisions.count() < 2 ) {
      REQUIRE( engine.advance() );
    }
  }

  /// The tokens awaiting entry at the performer.
  std::vector<const Execution::Token*> waitingTokens() const {
    std::vector<const Execution::Token*> tokens;
    for ( auto& [token_ptr, request_ptr] : engine.getSystemState()->pendingEntryDecisions ) {
      tokens.push_back(token_ptr.lock().get());
    }
    return tokens;
  }

  /// Advances the engine until the clock has ticked.
  void tick() {
    auto time = engine.getSystemState()->getTime();
    while ( engine.getSystemState()->getTime() == time ) {
      REQUIRE( engine.advance() );
    }
  }
};

} // namespace

SCENARIO( "Deferring the entries of a performer", "[controller][deferral]" ) {

  GIVEN( "A performer whose deferral observes x" ) {
    Run run(modelWith(R"(<bpmnos:deferral condition="x &lt; 2"><bpmnos:parameter name="observable" value="x" /></bpmnos:deferral>)"));
    auto tokens = run.waitingTokens();
    REQUIRE( tokens.size() == 2 );
    REQUIRE( !run.deferrals.isDeferred(tokens[0]) );

    WHEN( "A task awaiting entry is deferred" ) {
      run.deferrals.defer(tokens[0]);
      auto performerToken = tokens[0]->getSequentialPerformerToken();

      THEN( "The performer and every task awaiting entry at it are deferred, with the entry requests of both" ) {
        REQUIRE( run.deferrals.isDeferred(tokens[0]) );
        REQUIRE( run.deferrals.isDeferred(tokens[1]) );
        REQUIRE( run.deferrals.isDeferred(performerToken) );
        REQUIRE( run.deferrals.getDeferrals().size() == 1 );
        REQUIRE( std::get<0>(run.deferrals.getDeferrals().at(performerToken)).size() == 2 );
      }

      THEN( "A new entry request at the performer ends the deferral" ) {
        run.deferrals.notice(tokens[1]->decisionRequest.get());
        REQUIRE( !run.deferrals.isDeferred(tokens[0]) );
      }

      THEN( "A data update naming x ends the deferral" ) {
        auto x = tokens[0]->node->extensionElements->as<Model::ExtensionElements>()->attributeRegistry["x"];
        std::vector<const Model::Attribute*> attributes = { x };
        Execution::DataUpdate update(tokens[0]->owner->root->instance.value(), attributes);
        run.deferrals.notice(&update);
        REQUIRE( !run.deferrals.isDeferred(tokens[0]) );
      }

      THEN( "A data update of another instance leaves the deferral" ) {
        auto x = tokens[0]->node->extensionElements->as<Model::ExtensionElements>()->attributeRegistry["x"];
        std::vector<const Model::Attribute*> attributes = { x };
        Execution::DataUpdate update(tokens[0]->owner->root->instance.value() + BPMNOS::number(1), attributes);
        run.deferrals.notice(&update);
        REQUIRE( run.deferrals.isDeferred(tokens[0]) );
      }

      THEN( "An installed system state ends the deferral" ) {
        run.deferrals.notice(run.engine.getSystemState());
        REQUIRE( run.deferrals.getDeferrals().empty() );
      }

      THEN( "The deferral is not admissible, nothing being pending, and ends at the next clock tick" ) {
        REQUIRE( !run.deferrals.isAdmissible(std::get<1>(run.deferrals.getDeferrals().begin()->second)) );
        run.tick();
        REQUIRE( !run.deferrals.isDeferred(tokens[0]) );
      }
    }
  }

  GIVEN( "A performer whose deferral holds until time 100" ) {
    Run run(modelWith(R"(<bpmnos:deferral condition="time &lt; 100" />)"));
    auto tokens = run.waitingTokens();
    run.deferrals.defer(tokens[0]);

    THEN( "The deferral is admissible, depending on time, and stays at the next clock tick" ) {
      run.tick();
      REQUIRE( run.deferrals.isDeferred(tokens[0]) );
    }
  }

  GIVEN( "A performer whose deferral holds before time 1" ) {
    Run run(modelWith(R"(<bpmnos:deferral condition="time &lt; 1" />)"));
    auto tokens = run.waitingTokens();
    REQUIRE( run.engine.getSystemState()->getTime() < 1 );
    run.deferrals.defer(tokens[0]);

    THEN( "The deferral ends at the clock tick at which its condition no longer holds" ) {
      while ( run.engine.getSystemState()->getTime() < 1 ) {
        REQUIRE( run.deferrals.isDeferred(tokens[0]) );
        run.tick();
      }
      REQUIRE( !run.deferrals.isDeferred(tokens[0]) );
    }
  }

  GIVEN( "A performer without deferral" ) {
    Run run(modelWith(""));
    THEN( "Its entries cannot be deferred" ) {
      REQUIRE_THROWS( run.deferrals.defer(run.waitingTokens()[0]) );
    }
  }
}

namespace {

/// Returns the number of tokens of the instance awaiting a decision of the given pending list.
template <typename PendingList>
size_t pendingOf(const PendingList& pending, const std::string& instance) {
  size_t count = 0;
  for ( auto& [token_ptr, request_ptr] : pending ) {
    if ( auto token = token_ptr.lock(); token && token->owner->root->instance.value() == BPMNOS::to_number(instance, STRING) ) {
      count++;
    }
  }
  return count;
}

/// A decision maker deferring the performer of the tasks of an instance at every entry request of these tasks before the
/// given time, the deferral being placed after a request has ended the previous one.
struct DeferringEntries : Execution::Observer {
  Execution::Deferrals& deferrals;
  BPMNOS::number instance;
  BPMNOS::number until;

  DeferringEntries(Execution::Deferrals& deferrals, const std::string& instance, BPMNOS::number until)
    : deferrals(deferrals)
    , instance(BPMNOS::to_number(instance, STRING))
    , until(until)
  {
  }

  void notice(const Execution::Observable* observable) override {
    auto token = static_cast<const Execution::DecisionRequest*>(observable)->token;
    if ( token->owner->root->instance.value() == instance && token->owner->systemState->getTime() < until ) {
      deferrals.defer(token);
    }
  }
};

} // namespace

SCENARIO( "A greedy controller withholds the candidates of deferred entries", "[controller][deferral][greedy]" ) {
  auto model = modelWith(R"(<bpmnos:deferral condition="time &lt; 3" />)");
  auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model,
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "Instance_1; Process_1; timestamp := 0\n"
    "Instance_2; Process_1; timestamp := 0\n"
  );
  Execution::Deferrals deferrals;
  Execution::GreedyController controller(std::make_shared<Execution::LocalEvaluator>(), deferrals);
  Execution::Engine engine(model);
  controller.connect(&engine);
  // subscribed after the controller, so that the deferral is placed after the deferrals have noticed the request,
  // which ends the deferral placed at an earlier request of the same performer
  DeferringEntries deferring(deferrals, "Instance_1", BPMNOS::number(3));
  engine.addSubscriber(&deferring, Execution::Observable::Type::EntryRequest);
  engine.initialize(dataProvider->createScenario(), 0);

  THEN( "The deferred tasks await entry while the deferral holds, the other instance proceeding" ) {
    while ( engine.getSystemState()->getTime() < 3 ) {
      // at time 0 the tasks become ready one after the other
      if ( engine.getSystemState()->getTime() >= 1 ) {
        REQUIRE( pendingOf(engine.getSystemState()->pendingEntryDecisions, "Instance_1") == 2 );
        REQUIRE( pendingOf(engine.getSystemState()->pendingEntryDecisions, "Instance_2") == 0 );
      }
      REQUIRE( engine.advance() );
    }
    AND_THEN( "A task of the deferred instance is entered once the condition no longer holds" ) {
      while ( pendingOf(engine.getSystemState()->pendingEntryDecisions, "Instance_1") == 2 ) {
        REQUIRE( engine.advance() );
      }
      REQUIRE( engine.getSystemState()->getTime() == 3 );
    }
  }
}

SCENARIO( "A greedy controller withholds the candidates of a deferred choice", "[controller][deferral][greedy]" ) {
  std::ifstream file("tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn");
  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string xml = buffer.str();
  // the choice guidance of the decision task states a deferral holding before time 3
  auto position = xml.find("</bpmnos:status>", xml.find("Decision_27n2nne"));
  REQUIRE( position != std::string::npos );
  position += std::string("</bpmnos:status>").size();
  xml.insert(position, R"(<bpmnos:guidance type="choice"><bpmnos:deferral condition="time &lt; 3" /></bpmnos:guidance>)");
  auto root = XML::XMLObject::createFromString(xml);
  REQUIRE( root );
  auto model = std::make_shared<const Model::Model>(std::unique_ptr<XML::XMLObject>(root), std::unordered_map<std::string, std::string>{});

  auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model,
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "Instance_1; Process_1;\n"
    "Instance_1; Activity_1; x := -2\n"
  );
  Execution::Deferrals deferrals;
  Execution::GreedyController controller(std::make_shared<Execution::LocalEvaluator>(), deferrals);
  Execution::Engine engine(model);
  controller.connect(&engine);
  engine.initialize(dataProvider->createScenario(), 0);

  while ( engine.getSystemState()->pendingChoiceDecisions.count() == 0 ) {
    REQUIRE( engine.advance() );
  }
  for ( auto& [token_ptr, request_ptr] : engine.getSystemState()->pendingChoiceDecisions ) {
    deferrals.defer(token_ptr.lock().get());
  }

  THEN( "The choice is made at the clock tick at which the condition no longer holds" ) {
    while ( engine.getSystemState()->getTime() < 3 ) {
      REQUIRE( engine.getSystemState()->pendingChoiceDecisions.count() == 1 );
      REQUIRE( engine.advance() );
    }
    while ( engine.getSystemState()->pendingChoiceDecisions.count() == 1 ) {
      REQUIRE( engine.advance() );
    }
    REQUIRE( engine.getSystemState()->getTime() == 3 );
  }
}
