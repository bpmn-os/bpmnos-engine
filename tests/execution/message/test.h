SCENARIO( "Simple messaging", "[execution][message]" ) {
  const std::string modelFile = "execution/message/Simple_messaging.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances starting at time 0" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,0\n"
      "Process_2, Instance_2,Timestamp,0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the message is delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "MessageThrowEvent_1"},{"state", "DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "MessageCatchEvent_2"},{"state", "DONE"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Two instances with mismatching id's starting at time 0" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,0\n"
      "Process_2, Instance_X,Timestamp,0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),0);
      THEN( "Then the message is not delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "MessageThrowEvent_1"},{"state", "DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_X"},{"nodeId", "MessageCatchEvent_2"},{"state", "BUSY"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_X"},{"nodeId", "MessageCatchEvent_2"},{"state", "COMPLETED"}}).size() == 0 );
      }
    }
  }

  GIVEN( "Two instances with mismatching start times" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,0\n"
      "Process_2, Instance_2,Timestamp,1\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),1);
      THEN( "Then the message is not delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "MessageThrowEvent_1"},{"state", "DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "MessageCatchEvent_2"},{"state", "BUSY"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "MessageCatchEvent_2"},{"state", "COMPLETED"}}).size() == 0 );
      }
    }
  }

  GIVEN( "Three instances" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,0\n"
      "Process_2, Instance_X,Timestamp,0\n"
      "Process_2, Instance_2,Timestamp,0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      timeHandler.subscribe(&engine);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),0);
      THEN( "Then the message is only delivered to one recipient" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "MessageThrowEvent_1"},{"state", "DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "MessageCatchEvent_2"},{"state", "BUSY"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_X"},{"nodeId", "MessageCatchEvent_2"},{"state", "COMPLETED"}}).size() == 0 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "MessageCatchEvent_2"},{"state", "COMPLETED"}}).size() == 1 );
      }
    }
  }
}


SCENARIO( "Message tasks", "[execution][message]" ) {
  const std::string modelFile = "execution/message/Message_tasks.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances without input" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
      "Process_2, Instance_2,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the message is delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "SendTask_1"},{"state", "COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "ReceiveTask_2"},{"state", "COMPLETED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Throwing instance without recipient" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the send task fails" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "SendTask_1"},{"state", "FAILED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Catching instance without sender" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_2, Instance_2,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the receive tasks fails" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "ReceiveTask_2"},{"state", "FAILED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Throwing instance waiting for recipient" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,0\n"
      "Process_2, Instance_2,Timestamp,1\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Both message tasks fail" ) {
        auto completionLog1 = recorder.find(nlohmann::json{{"processId", "Process_1"},{"state", "FAILED"}});
        REQUIRE( completionLog1.back()["status"]["timestamp"] == 0.0 );
        auto completionLog2 = recorder.find(nlohmann::json{{"processId", "Process_2"},{"state", "FAILED"}});
        REQUIRE( completionLog2.back()["status"]["timestamp"] == 1.0 );
      }
    }
  }

}

SCENARIO( "Message tasks with timer", "[execution][message]" ) {
  const std::string modelFile = "execution/message/Message_tasks_with_timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances without input" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
      "Process_2, Instance_2,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the message is delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "SendTask_1"},{"state", "COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "ReceiveTask_2"},{"state", "COMPLETED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Throwing instance without recipient" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the message is withdrawn" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "SendTask_1"},{"state", "WITHDRAWN"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Catching instance without sender" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_2, Instance_2,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the message is withdrawn" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "ReceiveTask_2"},{"state", "WITHDRAWN"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Throwing instance waiting for recipient" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,0\n"
      "Process_2, Instance_2,Timestamp,1\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntry entryHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      Execution::TimeWarp timeHandler;
      messageHandler.subscribe(&engine);
      readyHandler.subscribe(&engine);
      entryHandler.subscribe(&engine);
      completionHandler.subscribe(&engine);
      exitHandler.subscribe(&engine);
      messageTaskTerminator.subscribe(&engine);
      timeHandler.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the message is delivered at time 1" ) {
        auto completionLog1 = recorder.find(nlohmann::json{{"processId", "Process_1"},{"state", "COMPLETED"}});
        REQUIRE( completionLog1.back()["status"]["timestamp"] == 1.0 );
        auto completionLog2 = recorder.find(nlohmann::json{{"processId", "Process_2"},{"state", "COMPLETED"}});
        REQUIRE( completionLog2.back()["status"]["timestamp"] == 1.0 );
      }
    }

  }

}
