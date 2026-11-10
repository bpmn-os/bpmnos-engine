#include "prelude.h"

namespace {

/// Returns the number of decisions of each kind a recorded log contains, and for each time the number of decisions
/// belonging to it, which is the time of the last clock tick before the decision.
std::pair< std::map<std::string, size_t>, std::map<double, size_t> > countDecisions(const nlohmann::ordered_json& log) {
  std::map<std::string, size_t> kinds;
  std::map<double, size_t> times;
  double time = std::numeric_limits<double>::lowest();
  for ( auto& record : log ) {
    if ( !record.contains("event") ) {
      continue;
    }
    auto& event = record["event"];
    if ( event.contains("event") && event["event"] == "clocktick" ) {
      time = event["time"].get<double>();
    }
    else if ( event.contains("decision") ) {
      kinds[event["decision"].get<std::string>()]++;
      times[time]++;
    }
  }
  return { kinds, times };
}

/// Returns the number of decisions of each kind and for each time that a playback controller has read.
std::pair< std::map<std::string, size_t>, std::map<double, size_t> > countDecisions(const Execution::PlaybackController& controller) {
  std::map<std::string, size_t> kinds;
  std::map<double, size_t> times;
  for ( auto& [time, decisions] : controller.getDecisions() ) {
    for ( auto& decision : decisions ) {
      switch ( decision.kind ) {
        case Execution::PlaybackController::LoggedDecision::Kind::Entry:
          kinds["entry"]++;
          break;
        case Execution::PlaybackController::LoggedDecision::Kind::Choice:
          kinds["choice"]++;
          break;
        case Execution::PlaybackController::LoggedDecision::Kind::MessageDelivery:
          kinds["messagedelivery"]++;
          break;
      }
      times[(double)time]++;
    }
  }
  return { kinds, times };
}

} // namespace

SCENARIO( "Playback of an empty log", "[playback]" ) {
  const std::string modelFile = "tests/execution/task/Task_with_linear_expression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A model without decisions and a log without decisions" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    WHEN( "The engine is started with a playback controller" ) {
      Execution::Engine engine(model);
      Execution::PlaybackController controller(nlohmann::ordered_json::array());
      controller.connect(&engine);
      Execution::OutcomeSentinel sentinel;
      sentinel.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The run completes" ) {
        REQUIRE( controller.getDecisions().empty() );
        REQUIRE( sentinel.getOutcome() == Execution::Outcome::COMPLETED );
      }
    }
  }
}

SCENARIO( "Reading the decisions of a log recorded by the greedy controller", "[playback]" ) {
  GIVEN( "A job shop scheduling problem, whose entries and message deliveries are decided" ) {
    const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
    REQUIRE_NOTHROW( Model::Model(modelFile) );
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Machine2; MachineProcess; jobs := 3\n"
      "Machine3; MachineProcess; jobs := 3\n"
      "Order1; OrderProcess; machines := [\"Machine1\",\"Machine2\",\"Machine3\"]\n"
      "Order1; OrderProcess; durations := [3.0,2.0,2.0]\n"
      "Order2; OrderProcess; machines := [\"Machine1\",\"Machine3\",\"Machine2\"]\n"
      "Order2; OrderProcess; durations := [2.0,1.0,4.0]\n"
      "Order3; OrderProcess; machines := [\"Machine2\",\"Machine3\"]\n"
      "Order3; OrderProcess; durations := [4.0,3.0]\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    WHEN( "A log recorded by the greedy controller is read" ) {
      Execution::Engine engine(model);
      auto evaluator = std::make_shared<Execution::GuidedEvaluator>();
      Execution::GreedyController greedy(evaluator);
      greedy.connect(&engine);
      Execution::Recorder recorder(Execution::Recorder::Config{ .stream = std::nullopt, .tagged = true });
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      Execution::PlaybackController controller(recorder.log);
      auto [loggedKinds, loggedTimes] = countDecisions(recorder.log);
      auto [readKinds, readTimes] = countDecisions(controller);

      THEN( "Every decision of the log is read, at the time of the last clock tick before it" ) {
        REQUIRE( loggedKinds["entry"] > 0 );
        REQUIRE( loggedKinds["messagedelivery"] > 0 );
        REQUIRE( readKinds == loggedKinds );
        REQUIRE( readTimes == loggedTimes );
      }
      AND_THEN( "Every message delivery read names the origin of the message" ) {
        for ( auto& [time, decisions] : controller.getDecisions() ) {
          for ( auto& decision : decisions ) {
            if ( decision.kind == Execution::PlaybackController::LoggedDecision::Kind::MessageDelivery ) {
              REQUIRE_FALSE( decision.origin.empty() );
            }
          }
        }
      }
    }
  }

  GIVEN( "A decision task, whose choice is decided" ) {
    const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn";
    REQUIRE_NOTHROW( Model::Model(modelFile) );
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := 4\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    WHEN( "A log recorded by the greedy controller is read" ) {
      Execution::Engine engine(model);
      auto evaluator = std::make_shared<Execution::LocalEvaluator>();
      Execution::GreedyController greedy(evaluator);
      greedy.connect(&engine);
      Execution::Recorder recorder(Execution::Recorder::Config{ .stream = std::nullopt, .tagged = true });
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      Execution::PlaybackController controller(recorder.log);
      auto [loggedKinds, loggedTimes] = countDecisions(recorder.log);
      auto [readKinds, readTimes] = countDecisions(controller);

      THEN( "The choice is read with the values chosen" ) {
        REQUIRE( loggedKinds["choice"] == 1 );
        REQUIRE( readKinds == loggedKinds );
        REQUIRE( readTimes == loggedTimes );
        for ( auto& [time, decisions] : controller.getDecisions() ) {
          for ( auto& decision : decisions ) {
            if ( decision.kind == Execution::PlaybackController::LoggedDecision::Kind::Choice ) {
              REQUIRE( decision.nodeId == "Activity_1" );
              REQUIRE( decision.choices.size() == 1 );
            }
          }
        }
      }
    }
  }

  GIVEN( "Logs that are not logs of decisions" ) {
    THEN( "A log that is not an array is refused" ) {
      REQUIRE_THROWS_WITH( Execution::PlaybackController(nlohmann::ordered_json::object()), "PlaybackController: the log must be an array of records" );
    }
    THEN( "A decision of an unknown kind is refused" ) {
      auto log = nlohmann::ordered_json::parse(R"([ {"event": {"decision": "exit", "processId": "P", "instanceId": "I", "nodeId": "N"}} ])");
      REQUIRE_THROWS_WITH( Execution::PlaybackController(log), "PlaybackController: decision of unknown kind 'exit'" );
    }
    THEN( "An entry without its node is refused" ) {
      auto log = nlohmann::ordered_json::parse(R"([ {"event": {"decision": "entry", "processId": "P", "instanceId": "I"}} ])");
      REQUIRE_THROWS_WITH( Execution::PlaybackController(log), "PlaybackController: decision without field 'nodeId'" );
    }
    THEN( "A log stating its decisions alone is read" ) {
      auto log = nlohmann::ordered_json::parse(R"([
        {"event": {"decision": "entry", "processId": "P", "instanceId": "I", "nodeId": "A"}},
        {"event": {"event": "clocktick", "time": 2}},
        {"event": {"decision": "choice", "processId": "P", "instanceId": "I", "nodeId": "B", "choices": {"x": 1}}},
        {"event": {"decision": "messagedelivery", "processId": "P", "instanceId": "I", "nodeId": "C", "message": {"origin": "D", "header": {"sender": "J"}}}}
      ])");
      Execution::PlaybackController controller(log);
      auto& decisions = controller.getDecisions();
      REQUIRE( decisions.size() == 2 );
      REQUIRE( decisions.begin()->first == std::numeric_limits<BPMNOS::number>::lowest() );
      REQUIRE( decisions.begin()->second.size() == 1 );
      auto& atTwo = decisions.at(BPMNOS::number(2));
      REQUIRE( atTwo.size() == 2 );
      REQUIRE( atTwo[0].kind == Execution::PlaybackController::LoggedDecision::Kind::Choice );
      REQUIRE( atTwo[0].choices["x"] == 1 );
      REQUIRE( atTwo[1].kind == Execution::PlaybackController::LoggedDecision::Kind::MessageDelivery );
      REQUIRE( atTwo[1].origin == "D" );
      REQUIRE( atTwo[1].sender == "J" );
    }
  }
}

namespace {

/// Runs the greedy controller on a scenario of the data provider and returns its log and objective.
std::pair<nlohmann::ordered_json, BPMNOS::number> runGreedy(std::shared_ptr<const Model::Model> model, std::shared_ptr<Execution::StaticDataProvider> dataProvider) {
  Execution::Engine engine(model);
  auto evaluator = std::make_shared<Execution::GuidedEvaluator>();
  Execution::GreedyController greedy(evaluator);
  greedy.connect(&engine);
  Execution::Recorder recorder(Execution::Recorder::Config{ .stream = std::nullopt, .tagged = true });
  recorder.subscribe(&engine);
  engine.run(dataProvider->createScenario());
  return { recorder.log, engine.getSystemState()->getObjective() };
}

/// Plays back a log on a scenario of the data provider and returns the objective of the run.
BPMNOS::number runPlayback(std::shared_ptr<const Model::Model> model, std::shared_ptr<Execution::StaticDataProvider> dataProvider, Execution::PlaybackController& controller) {
  Execution::Engine engine(model);
  controller.connect(&engine);
  engine.run(dataProvider->createScenario());
  return engine.getSystemState()->getObjective();
}

} // namespace

SCENARIO( "Playback of logs recorded by the greedy controller", "[playback]" ) {
  GIVEN( "A job shop scheduling problem, whose entries and message deliveries are decided" ) {
    const std::string modelFile = "examples/job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn";
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Machine1; MachineProcess; jobs := 2\n"
      "Machine2; MachineProcess; jobs := 3\n"
      "Machine3; MachineProcess; jobs := 3\n"
      "Order1; OrderProcess; machines := [\"Machine1\",\"Machine2\",\"Machine3\"]\n"
      "Order1; OrderProcess; durations := [3.0,2.0,2.0]\n"
      "Order2; OrderProcess; machines := [\"Machine1\",\"Machine3\",\"Machine2\"]\n"
      "Order2; OrderProcess; durations := [2.0,1.0,4.0]\n"
      "Order3; OrderProcess; machines := [\"Machine2\",\"Machine3\"]\n"
      "Order3; OrderProcess; durations := [4.0,3.0]\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto [log, objective] = runGreedy(model, dataProvider);

    WHEN( "The log is played back" ) {
      Execution::PlaybackController controller(log);
      auto playedObjective = runPlayback(model, dataProvider, controller);
      THEN( "Every decision is dispatched at its time and the objective is reproduced" ) {
        REQUIRE( controller.getDispatched() > 0 );
        REQUIRE( controller.isComplete() );
        REQUIRE( controller.getDeviations().empty() );
        REQUIRE( playedObjective == objective );
      }
    }
  }

  GIVEN( "A travelling salesperson problem, whose visits are decided" ) {
    const std::string modelFile = "examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn";
    const std::vector<std::string> folders = { "tests/examples/travelling_salesperson_problem" };
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance1; TravellingSalesperson_Process; speed := 1\n"
      "Instance1; TravellingSalesperson_Process; origin := \"Hamburg\"\n"
      "Instance1; TravellingSalesperson_Process; locations := [\"Munich\",\"Berlin\",\"Cologne\"]\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile, folders);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto [log, objective] = runGreedy(model, dataProvider);

    WHEN( "The log is played back" ) {
      Execution::PlaybackController controller(log);
      auto playedObjective = runPlayback(model, dataProvider, controller);
      THEN( "Every decision is dispatched and the objective is reproduced" ) {
        REQUIRE( controller.getDispatched() > 0 );
        REQUIRE( controller.isComplete() );
        REQUIRE( playedObjective == objective );
      }
    }
  }

  GIVEN( "A decision task, whose choice is decided" ) {
    const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn";
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := 4\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto [log, objective] = runGreedy(model, dataProvider);

    // the choice as the greedy controller has recorded it
    nlohmann::ordered_json choice;
    for ( auto& record : log ) {
      if ( record.contains("event") && record["event"].contains("decision") ) {
        choice = record;
      }
    }
    REQUIRE( choice["event"]["decision"] == "choice" );

    WHEN( "The log is played back" ) {
      Execution::PlaybackController controller(log);
      auto playedObjective = runPlayback(model, dataProvider, controller);
      THEN( "The choice is dispatched and the objective is reproduced" ) {
        REQUIRE( controller.getDispatched() == 1 );
        REQUIRE( controller.isComplete() );
        REQUIRE( playedObjective == objective );
      }
    }

    WHEN( "A log states the choice at a time between two clock ticks" ) {
      auto offGrid = nlohmann::ordered_json::array();
      offGrid.push_back( nlohmann::ordered_json{{"event", {{"event","clocktick"},{"time",0.5}}}} );
      offGrid.push_back( choice );
      Execution::PlaybackController controller(offGrid);
      runPlayback(model, dataProvider, controller);
      THEN( "The choice is dispatched at the first clock tick after its time and the deviation is reported" ) {
        REQUIRE( controller.isComplete() );
        REQUIRE( controller.getDeviations().size() == 1 );
        REQUIRE( controller.getDeviations().front().time == BPMNOS::number(1) );
        REQUIRE( controller.jsonify()["deviations"][0]["dispatchedAt"] == 1.0 );
      }
    }

    WHEN( "A log states the choice for an instance that does not exist" ) {
      auto altered = nlohmann::ordered_json::array();
      auto unknown = choice;
      unknown["event"]["instanceId"] = "Instance_2";
      altered.push_back( unknown );
      Execution::PlaybackController controller(altered);
      Execution::Engine engine(model);
      controller.connect(&engine);
      Execution::OutcomeSentinel sentinel;
      sentinel.subscribe(&engine);
      engine.run(dataProvider->createScenario());
      THEN( "The playback stops at the decision not dispatched at its time" ) {
        REQUIRE_FALSE( controller.isComplete() );
        REQUIRE( controller.getMismatch().has_value() );
        REQUIRE( controller.getMismatch()->decision->instanceId == "Instance_2" );
        REQUIRE( controller.getMismatch()->reason.starts_with("not dispatched at time") );
        REQUIRE( sentinel.getOutcome() == Execution::Outcome::TERMINATED );
      }
    }

    WHEN( "A log states the choice without the value of its attribute" ) {
      auto altered = nlohmann::ordered_json::array();
      auto incomplete = choice;
      incomplete["event"]["choices"] = nlohmann::ordered_json::object();
      altered.push_back( incomplete );
      Execution::PlaybackController controller(altered);
      runPlayback(model, dataProvider, controller);
      THEN( "The playback stops at the choice" ) {
        REQUIRE( controller.getMismatch().has_value() );
        REQUIRE( controller.getMismatch()->reason.starts_with("choice without value of attribute") );
      }
    }
  }

  GIVEN( "A log stating the entry of an activity whose entry is not decided" ) {
    const std::string modelFile = "tests/execution/task/Task_with_linear_expression.bpmn";
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto log = nlohmann::ordered_json::parse(R"([ {"event": {"decision": "entry", "processId": "Process_1", "instanceId": "Instance_1", "nodeId": "Activity_1"}} ])");

    WHEN( "The log is played back" ) {
      Execution::PlaybackController controller(log);
      runPlayback(model, dataProvider, controller);
      THEN( "The playback stops at the entry" ) {
        REQUIRE( controller.getMismatch().has_value() );
        REQUIRE( controller.getMismatch()->reason == "entry of an activity whose entry is not decided" );
      }
    }
  }
}
