#include "prelude.h"

SCENARIO( "Scenarios created by a data provider held through its base", "[data][forking]" ) {
  auto model = std::make_shared<const Model::Model>("tests/execution/process/Empty_executable_process.bpmn");
  std::string csv =
    "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
    "Instance_1; Process_1; timestamp := uniform(0,100)\n"
  ;

  GIVEN( "A stochastic data provider held as a static data provider" ) {
    std::shared_ptr<Execution::StaticDataProvider> dataProvider = std::make_shared<Execution::StochasticDataProvider>(model, csv);

    THEN( "It creates a scenario of the stochastic data provider" ) {
      auto scenario = dataProvider->createScenario();
      REQUIRE( dynamic_cast<Execution::StochasticDataProvider::Scenario*>(scenario.get()) != nullptr );
    }
  }
}

SCENARIO( "Forks of a deterministic run", "[data][forking]" ) {
  auto model = std::make_shared<const Model::Model>("tests/execution/task/Task_with_linear_expression.bpmn");

  GIVEN( "A static and a dynamic data provider" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    std::vector<std::shared_ptr<Execution::StaticDataProvider>> dataProviders = {
      std::make_shared<Execution::StaticDataProvider>(model, csv),
      std::make_shared<Execution::DynamicDataProvider>(model, csv)
    };

    // the token log of a run of the scenario from the given state, or from the start
    auto tokenLog = [&](std::unique_ptr<Execution::Scenario> scenario, const Execution::SystemState* state) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      if ( state ) {
        engine.initializeSystemState(std::move(scenario), state);
        engine.resume();
      }
      else {
        engine.run(std::move(scenario));
      }
      return recorder.find(nlohmann::json{{"nodeId","Activity_1"}}, nlohmann::json{{"event",nullptr},{"decision",nullptr}});
    };

    THEN( "The fork of a run stopped while the task is busy realises the same events as the run" ) {
      for ( auto& dataProvider : dataProviders ) {
        Execution::Engine engine(model);
        Execution::InstantEntry entryHandler;
        Execution::InstantExit exitHandler;
        entryHandler.connect(&engine);
        exitHandler.connect(&engine);
        dataProvider->setEndTime(0);
        engine.run(dataProvider->createScenario());
        dataProvider->setEndTime(std::numeric_limits<BPMNOS::number>::max());

        auto runLog = tokenLog(dataProvider->createScenario(), nullptr);
        auto forkLog = tokenLog(dataProvider->forkScenario(*engine.getSystemState()->scenario, 1), engine.getSystemState());
        REQUIRE_FALSE( forkLog.empty() );
        REQUIRE( forkLog.back()["state"] == "DEPARTED" );
        REQUIRE( forkLog.back()["status"] == runLog.back()["status"] );
      }
    }
  }
}

SCENARIO( "Forks of a stochastic run", "[data][forking]" ) {
  auto model = std::make_shared<const Model::Model>("tests/execution/process/Empty_executable_process.bpmn");

  GIVEN( "Instances with random timestamps, of which one is disclosed before the spawn time" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_0; Process_1; timestamp := uniform(20,100); 5\n"
      "Instance_1; Process_1; timestamp := uniform(20,100); 15\n"
      "Instance_2; Process_1; timestamp := uniform(20,100); 15\n"
      "Instance_3; Process_1; timestamp := uniform(20,100); 15\n"
      "Instance_4; Process_1; timestamp := uniform(20,100); 15\n"
    ;
    auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(model, csv, 7);

    // the run stops at time 10, so that the spawn time of a fork is 11
    Execution::Engine engine(model);
    dataProvider->setEndTime(10);
    engine.run(dataProvider->createScenario());
    REQUIRE( engine.getSystemState()->getTime() == 10 );
    dataProvider->setEndTime(std::numeric_limits<BPMNOS::number>::max());

    // the times at which the processes of the instances become ready when a fork with the given index is run
    auto readyTimes = [&](unsigned int index) {
      Execution::Engine forkEngine(model);
      Execution::Recorder recorder;
      recorder.subscribe(&forkEngine);
      forkEngine.initializeSystemState(dataProvider->forkScenario(*engine.getSystemState()->scenario, index), engine.getSystemState());
      forkEngine.resume();
      std::map<std::string, double> result;
      for ( auto& entry : recorder.find(nlohmann::json{{"state","READY"}}, nlohmann::json{{"nodeId",nullptr}}) ) {
        result[entry["instanceId"].get<std::string>()] = entry["status"]["timestamp"].get<double>();
      }
      return result;
    };

    WHEN( "Forks are run" ) {
      auto first = readyTimes(0);
      auto second = readyTimes(0);
      auto other = readyTimes(1);

      THEN( "Every instance is started in every fork" ) {
        REQUIRE( first.size() == 5 );
        REQUIRE( other.size() == 5 );
      }
      THEN( "Forks with one index realise the same events" ) {
        REQUIRE( first == second );
      }
      THEN( "Forks with different indices agree before the spawn time" ) {
        REQUIRE( first.at("Instance_0") == other.at("Instance_0") );
      }
      THEN( "Forks with different indices differ after the spawn time" ) {
        REQUIRE( first != other );
      }
    }
  }

  GIVEN( "A task whose completion sampled anew may lie before the current time of the run" ) {
    auto taskModel = std::make_shared<const Model::Model>("tests/execution/task/Task_with_linear_expression.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1;;;;\n"
      "Instance_1; Activity_1;;;; timestamp := timestamp + uniform(0,1000)\n"
    ;

    WHEN( "Runs stopped at time 500 while the task is busy are forked" ) {
      // the completion times of the forks of every run, among runs with several seeds, still busy at time 500
      std::vector<double> completionTimes;
      for ( unsigned int seed = 1; seed <= 20; seed++ ) {
        auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(taskModel, csv, seed);
        Execution::Engine engine(taskModel);
        Execution::InstantEntry entryHandler;
        Execution::InstantExit exitHandler;
        entryHandler.connect(&engine);
        exitHandler.connect(&engine);
        Execution::Recorder recorder;
        recorder.subscribe(&engine);
        dataProvider->setEndTime(500);
        engine.run(dataProvider->createScenario());
        if ( !recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","COMPLETED"}}).empty() ) {
          // the task completed before time 500
          continue;
        }
        dataProvider->setEndTime(std::numeric_limits<BPMNOS::number>::max());
        for ( unsigned int index = 0; index < 10; index++ ) {
          Execution::Engine forkEngine(taskModel);
          Execution::InstantEntry forkEntryHandler;
          Execution::InstantExit forkExitHandler;
          forkEntryHandler.connect(&forkEngine);
          forkExitHandler.connect(&forkEngine);
          Execution::Recorder forkRecorder;
          forkRecorder.subscribe(&forkEngine);
          forkEngine.initializeSystemState(dataProvider->forkScenario(*engine.getSystemState()->scenario, index), engine.getSystemState());
          forkEngine.resume();
          auto completionLog = forkRecorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","COMPLETED"}});
          REQUIRE( completionLog.size() == 1 );
          completionTimes.push_back(completionLog.front()["status"]["timestamp"].get<double>());
        }
      }

      THEN( "Every fork completes the task not before the current time of the run" ) {
        REQUIRE_FALSE( completionTimes.empty() );
        for ( auto completionTime : completionTimes ) {
          REQUIRE( completionTime >= 500 );
        }
      }
    }
  }

  GIVEN( "A task without duration becoming busy in a fork" ) {
    auto taskModel = std::make_shared<const Model::Model>("tests/execution/process/Simple_executable_process.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1;;;;\n"
      "Instance_1; Activity_1;;;; timestamp := timestamp\n"
    ;
    auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(taskModel, csv, 7);

    WHEN( "A run stops at time 0 with the token awaiting the decision to enter the task and is forked" ) {
      // without an entry handler the token awaits the entry decision, and the run ends at time 0
      Execution::Engine engine(taskModel);
      dataProvider->setEndTime(0);
      engine.run(dataProvider->createScenario());
      REQUIRE_FALSE( engine.getSystemState()->pendingEntryDecisions.empty() );
      dataProvider->setEndTime(std::numeric_limits<BPMNOS::number>::max());

      Execution::Engine forkEngine(taskModel);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&forkEngine);
      exitHandler.connect(&forkEngine);
      Execution::Recorder recorder;
      recorder.subscribe(&forkEngine);
      forkEngine.initializeSystemState(dataProvider->forkScenario(*engine.getSystemState()->scenario, 0), engine.getSystemState());
      forkEngine.resume();

      THEN( "The task entered in the fork completes at the current time, as in any run" ) {
        auto completionLog = recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","COMPLETED"}});
        REQUIRE( completionLog.size() == 1 );
        REQUIRE( completionLog.front()["status"]["timestamp"] == 0.0 );
        REQUIRE( recorder.find(nlohmann::json{{"event","clocktick"}}).empty() );
      }
    }
  }

  GIVEN( "A task whose completion is random" ) {
    auto taskModel = std::make_shared<const Model::Model>("tests/execution/task/Task_with_linear_expression.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE; READY; COMPLETION\n"
      "Instance_1; Process_1;;;;\n"
      "Instance_1; Activity_1;;;; timestamp := timestamp + uniform(10,1000)\n"
    ;
    auto dataProvider = std::make_shared<Execution::StochasticDataProvider>(taskModel, csv, 7);

    // the time at which the task completes when a scenario is run from the given state, or from the start
    auto completionTime = [&](std::unique_ptr<Execution::Scenario> scenario, const Execution::SystemState* state) {
      Execution::Engine taskEngine(taskModel);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&taskEngine);
      exitHandler.connect(&taskEngine);
      Execution::Recorder recorder;
      recorder.subscribe(&taskEngine);
      if ( state ) {
        taskEngine.initializeSystemState(std::move(scenario), state);
        taskEngine.resume();
      }
      else {
        taskEngine.run(std::move(scenario));
      }
      auto completionLog = recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","COMPLETED"}});
      REQUIRE( completionLog.size() == 1 );
      return completionLog.front()["status"]["timestamp"].get<double>();
    };

    WHEN( "The run stops while the task is busy and is forked" ) {
      Execution::Engine engine(taskModel);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      dataProvider->setEndTime(5);
      engine.run(dataProvider->createScenario());
      REQUIRE( engine.getSystemState()->getTime() == 5 );
      dataProvider->setEndTime(std::numeric_limits<BPMNOS::number>::max());

      auto runCompletion = completionTime(dataProvider->createScenario(), nullptr);
      auto forkCompletion = completionTime(dataProvider->forkScenario(*engine.getSystemState()->scenario, 0), engine.getSystemState());

      THEN( "The task completes differently in the fork than in the run it is forked from" ) {
        REQUIRE( forkCompletion != runCompletion );
      }
    }
  }
}
