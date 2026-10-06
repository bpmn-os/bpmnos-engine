SCENARIO( "Static data provider", "[data][static]" ) {

  GIVEN( "A single instance whose timer is triggered at time 10" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/timer/Timer.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; trigger := 10\n"
    ;
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    WHEN( "The engine runs on a scenario of the data provider without a clock dispatcher" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The instance is instantiated at the first instant of the run" ) {
        REQUIRE( recorder.log[0]["event"] == "clocktick" );
        REQUIRE( recorder.log[1]["event"] == "instantiation" );
      }
      THEN( "The timer is triggered at time 10" ) {
        auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}});
        REQUIRE( timerLog.size() == 1 );
        REQUIRE( timerLog.front()["status"]["timestamp"] == 10.0 );
      }
      THEN( "The run ends with a termination event once no instance is left" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        REQUIRE( engine.getSystemState()->instances.empty() );
      }
    }

    WHEN( "The end time is set to time 5" ) {
      dataProvider->setEndTime(5);
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The run ends at time 5 with the instance still running" ) {
        REQUIRE( recorder.log.back()["event"] == "termination" );
        REQUIRE( engine.getCurrentTime() == 5 );
        REQUIRE( engine.getSystemState()->instances.size() == 1 );
      }

      AND_WHEN( "The end time is raised and the run is resumed" ) {
        dataProvider->setEndTime(std::numeric_limits<BPMNOS::number>::max());
        engine.resume();

        THEN( "The timer is triggered at time 10" ) {
          auto timerLog = recorder.find(nlohmann::json{{"nodeId","TimerEvent_1"},{"state","COMPLETED"}});
          REQUIRE( timerLog.size() == 1 );
          REQUIRE( timerLog.front()["status"]["timestamp"] == 10.0 );
          REQUIRE( engine.getSystemState()->instances.empty() );
        }
      }
    }
  }

  GIVEN( "A single instance with timestamp 3" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/process/Empty_executable_process.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 3\n"
    ;
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    WHEN( "The engine runs on a scenario of the data provider" ) {
      Execution::Engine engine(model);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The instance is created at once and its process becomes ready at time 3" ) {
        auto createdLog = recorder.find(nlohmann::json{{"processId","Process_1"},{"state","CREATED"}});
        REQUIRE( createdLog.size() == 1 );
        auto readyLog = recorder.find(nlohmann::json{{"processId","Process_1"},{"state","READY"}});
        REQUIRE( readyLog.size() == 1 );
        REQUIRE( readyLog.front()["status"]["timestamp"] == 3.0 );
      }
    }
  }

  GIVEN( "A single instance with a task whose duration is given by a linear expression" ) {
    auto model = std::make_shared<const Model::Model>("tests/execution/task/Task_with_linear_expression.bpmn");
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);

    WHEN( "The engine runs on a scenario of the data provider" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::InstantExit exitHandler;
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The task completes at time 1, the timestamp its operator gives the status it becomes busy with" ) {
        auto completionLog = recorder.find(nlohmann::json{{"nodeId","Activity_1"},{"state","COMPLETED"}});
        REQUIRE( completionLog.size() == 1 );
        REQUIRE( completionLog.front()["status"]["timestamp"] == 1.0 );
        auto processLog = recorder.find(nlohmann::json{}, nlohmann::json{{"nodeId",nullptr},{"event",nullptr},{"decision",nullptr}});
        REQUIRE( processLog.back()["state"] == "DONE" );
      }
    }
  }
}
