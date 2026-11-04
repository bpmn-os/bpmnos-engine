#include "prelude.h"
#include <set>
#include <string>

SCENARIO( "Inclusive gateway with default flow", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_gateway_with_default_flow.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token departs along the default flow" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_C"},{"state","DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_A"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_B"}}).empty() );
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.back()["state"] == "DONE" );
      }
    }
  }

  GIVEN( "A single instance starting at time 1" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token departs along each sequence flow whose condition is satisfied" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_A"},{"state","DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_B"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_C"}}).empty() );
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.back()["state"] == "DONE" );
      }
    }
  }

  GIVEN( "A single instance starting at time 2" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 2\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token departs along each sequence flow whose condition is satisfied" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_A"},{"state","DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_B"},{"state","DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_C"}}).empty() );
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
        REQUIRE( processLog.back()["state"] == "DONE" );
      }
    }
  }
}

SCENARIO( "Inclusive gateway without default flow", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_gateway_without_default_flow.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started" ) {
      Execution::Engine engine(model);
      THEN( "The run is aborted, as no condition of a sequence flow out of the gateway is satisfied" ) {
        REQUIRE_THROWS_WITH(
          engine.run(std::move(scenario)),
          "StateMachine: no condition of a sequence flow out of inclusive gateway 'Gateway_1' is satisfied, and it has no default flow"
        );
      }
    }
  }
}

SCENARIO( "Predecessors of an inclusive merge", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merge.bpmn";
  Model::Model model(modelFile);
  REQUIRE( model.predecessors.size() == 1 );
  auto& [gateway, predecessors] = *model.predecessors.begin();
  REQUIRE( gateway->id == "Gateway_2" );
  std::set<std::string> nodeIds;
  for ( auto predecessor : predecessors ) {
    nodeIds.insert(predecessor->id);
  }
  REQUIRE( nodeIds == std::set<std::string>({"StartEvent_1","Gateway_1","Task_A","Task_B"}) );
}

SCENARIO( "Inclusive merge releasing a single token", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merge.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance taking one branch" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      // position of the first record of the token log matching a node and a state
      auto position = [&recorder](const std::string& nodeId, const std::string& state) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( recorder.log[i].contains("nodeId") && recorder.log[i]["nodeId"] == nodeId &&
            recorder.log[i].contains("state") && recorder.log[i]["state"] == state &&
            !recorder.log[i].contains("event") && !recorder.log[i].contains("decision")
          ) {
            return i;
          }
        }
        return recorder.log.size();
      };
      THEN( "The single token taking a branch is released without being merged" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Task_B"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).size() == 1 );
        REQUIRE( position("Gateway_2","WAITING") < position("Gateway_2","ENTERED") );
        REQUIRE( position("Gateway_2","ENTERED") < recorder.log.size() );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}

SCENARIO( "Inclusive merge merging two tokens", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merge.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance taking both branches" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 2\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      // position of the first record of the token log matching a node and a state
      auto position = [&recorder](const std::string& nodeId, const std::string& state) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( recorder.log[i].contains("nodeId") && recorder.log[i]["nodeId"] == nodeId &&
            recorder.log[i].contains("state") && recorder.log[i]["state"] == state &&
            !recorder.log[i].contains("event") && !recorder.log[i].contains("decision")
          ) {
            return i;
          }
        }
        return recorder.log.size();
      };
      THEN( "The tokens of both branches are merged once the later one has arrived" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 2 );
        auto entryLog = recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}});
        REQUIRE( entryLog.size() == 1 );
        REQUIRE( entryLog[0]["status"]["timestamp"] >= 5 );
        REQUIRE( position("Task_B","COMPLETED") < position("Gateway_2","ENTERED") );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}

SCENARIO( "Inclusive merge releasing a token after an interruption", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_release_after_interruption.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      // position of the first record of the token log matching a node and a state
      auto position = [&recorder](const std::string& nodeId, const std::string& state) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( recorder.log[i].contains("nodeId") && recorder.log[i]["nodeId"] == nodeId &&
            recorder.log[i].contains("state") && recorder.log[i]["state"] == state &&
            !recorder.log[i].contains("event") && !recorder.log[i].contains("decision")
          ) {
            return i;
          }
        }
        return recorder.log.size();
      };
      THEN( "The waiting token is released without being merged once the other branch is withdrawn" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Task_C"},{"state","COMPLETED"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).size() == 1 );
        REQUIRE( position("Task_C","WITHDRAWN") < position("Gateway_2","ENTERED") );
        REQUIRE( position("Gateway_2","ENTERED") < recorder.log.size() );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}

SCENARIO( "Inclusive merge merging tokens after an interruption", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merge_after_interruption.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      // position of the first record of the token log matching a node and a state
      auto position = [&recorder](const std::string& nodeId, const std::string& state) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( recorder.log[i].contains("nodeId") && recorder.log[i]["nodeId"] == nodeId &&
            recorder.log[i].contains("state") && recorder.log[i]["state"] == state &&
            !recorder.log[i].contains("event") && !recorder.log[i].contains("decision")
          ) {
            return i;
          }
        }
        return recorder.log.size();
      };
      THEN( "The waiting tokens are merged once the remaining branch is withdrawn" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Task_C"},{"state","COMPLETED"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).size() == 1 );
        REQUIRE( position("Task_C","WITHDRAWN") < position("Gateway_2","ENTERED") );
        REQUIRE( position("Gateway_2","ENTERED") < recorder.log.size() );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}

SCENARIO( "Inclusive merge released after an exclusive gateway leads away from it", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merge_after_exclusive_split.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance in which the exclusive gateway leads away from the merge" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      // position of the first record of the token log matching a node and a state
      auto position = [&recorder](const std::string& nodeId, const std::string& state) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( recorder.log[i].contains("nodeId") && recorder.log[i]["nodeId"] == nodeId &&
            recorder.log[i].contains("state") && recorder.log[i]["state"] == state &&
            !recorder.log[i].contains("event") && !recorder.log[i].contains("decision")
          ) {
            return i;
          }
        }
        return recorder.log.size();
      };
      THEN( "The waiting token is released without being merged once the exclusive gateway leads away from the merge" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_2"},{"state","DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).size() == 1 );
        REQUIRE( position("Task_X","COMPLETED") < position("Gateway_2","ENTERED") );
        REQUIRE( position("Gateway_2","ENTERED") < recorder.log.size() );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}

SCENARIO( "Inclusive merge merging after an exclusive gateway leads into it", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merge_after_exclusive_split.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance in which the exclusive gateway leads into the merge" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 1\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      // position of the first record of the token log matching a node and a state
      auto position = [&recorder](const std::string& nodeId, const std::string& state) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( recorder.log[i].contains("nodeId") && recorder.log[i]["nodeId"] == nodeId &&
            recorder.log[i].contains("state") && recorder.log[i]["state"] == state &&
            !recorder.log[i].contains("event") && !recorder.log[i].contains("decision")
          ) {
            return i;
          }
        }
        return recorder.log.size();
      };
      THEN( "The tokens are merged once the exclusive gateway leads into the merge" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","EndEvent_2"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).size() == 1 );
        REQUIRE( position("Task_X","COMPLETED") < position("Gateway_2","ENTERED") );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}

SCENARIO( "Inclusive merge with an armed boundary event leading into it", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merge_with_armed_boundary_event.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      // position of the first record of the token log matching a node and a state
      auto position = [&recorder](const std::string& nodeId, const std::string& state) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( recorder.log[i].contains("nodeId") && recorder.log[i]["nodeId"] == nodeId &&
            recorder.log[i].contains("state") && recorder.log[i]["state"] == state &&
            !recorder.log[i].contains("event") && !recorder.log[i].contains("decision")
          ) {
            return i;
          }
        }
        return recorder.log.size();
      };
      THEN( "An armed boundary event leading into the merge does not block it" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).size() == 1 );
        REQUIRE( position("Gateway_2","ENTERED") < position("Task_A","COMPLETED") );
        REQUIRE( position("Task_A","COMPLETED") < recorder.log.size() );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}

SCENARIO( "Inclusive merge after a subprocess", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merge_after_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      // position of the first record of the token log matching a node and a state
      auto position = [&recorder](const std::string& nodeId, const std::string& state) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( recorder.log[i].contains("nodeId") && recorder.log[i]["nodeId"] == nodeId &&
            recorder.log[i].contains("state") && recorder.log[i]["state"] == state &&
            !recorder.log[i].contains("event") && !recorder.log[i].contains("decision")
          ) {
            return i;
          }
        }
        return recorder.log.size();
      };
      THEN( "The merge waits for the token at the sibling subprocess" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).size() == 1 );
        REQUIRE( position("Task_S","COMPLETED") < position("Gateway_2","ENTERED") );
        REQUIRE( position("SubProcess_1","COMPLETED") < position("Gateway_2","ENTERED") );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}

SCENARIO( "Inclusive merges in sequence", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merges_in_sequence.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      // position of the first record of the token log matching a node and a state
      auto position = [&recorder](const std::string& nodeId, const std::string& state) {
        for ( size_t i = 0; i < recorder.log.size(); i++ ) {
          if ( recorder.log[i].contains("nodeId") && recorder.log[i]["nodeId"] == nodeId &&
            recorder.log[i].contains("state") && recorder.log[i]["state"] == state &&
            !recorder.log[i].contains("event") && !recorder.log[i].contains("decision")
          ) {
            return i;
          }
        }
        return recorder.log.size();
      };
      THEN( "The downstream merge waits for the token waiting at the upstream merge" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_3"},{"state","WAITING"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_3"},{"state","ENTERED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).size() == 1 );
        REQUIRE( position("Gateway_3","ENTERED") < position("Gateway_2","ENTERED") );
        REQUIRE( position("Gateway_2","ENTERED") < recorder.log.size() );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}

SCENARIO( "Inclusive merge cleared by an error event subprocess", "[execution][inclusivegateway]" ) {
  const std::string modelFile = "tests/execution/inclusivegateway/Inclusive_merge_cleared_by_error.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance starting at time 0" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      entryHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "The token waiting at the merge is withdrawn when an error event subprocess clears the scope" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","ErrorStartEvent_1"},{"state","COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WAITING"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","WITHDRAWN"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Gateway_2"},{"state","ENTERED"}}).empty() );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId","Task_B"},{"state","WITHDRAWN"}}).size() == 1 );
        REQUIRE( engine.getSystemState()->tokensAwaitingGatewayActivation.empty() );
      }
      auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr },{"event",nullptr },{"decision",nullptr }});
      REQUIRE( processLog.back()["state"] == "DONE" );
    }
  }
}
