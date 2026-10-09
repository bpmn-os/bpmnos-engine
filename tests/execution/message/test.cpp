#include "prelude.h"

SCENARIO( "Simple messaging", "[execution][message]" ) {
  const std::string modelFile = "tests/execution/message/Simple_messaging.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances starting at time 0" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the message is delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "MessageThrowEvent_1"},{"state", "DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "MessageCatchEvent_2"},{"state", "DONE"}}).size() == 1 );
      }
      AND_THEN( "The header of the message is reported with the type of each of its keys" ) {
        auto messageLog = recorder.find(nlohmann::json{{"origin", "MessageThrowEvent_1"},{"state", "CREATED"}});
        REQUIRE( messageLog.size() == 1 );
        REQUIRE( messageLog[0]["header"]["name"] == "Message" );
        REQUIRE( messageLog[0]["header"]["sender"] == "Instance_1" );
        REQUIRE( messageLog[0]["header"]["recipient"] == "Instance_2" );
        REQUIRE( messageLog[0]["header"]["time"] == "0" );
      }
    }
  }

  GIVEN( "Two instances with mismatching id's starting at time 0" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_X; Process_2; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(0);
      engine.run(std::move(scenario));
      THEN( "Then the message is not delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "MessageThrowEvent_1"},{"state", "DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_X"},{"nodeId", "MessageCatchEvent_2"},{"state", "BUSY"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_X"},{"nodeId", "MessageCatchEvent_2"},{"state", "COMPLETED"}}).size() == 0 );
      }
    }
  }

  GIVEN( "Two instances with mismatching parameters" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 1\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(1);
      engine.run(std::move(scenario));
      THEN( "Then the message is not delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "MessageThrowEvent_1"},{"state", "DONE"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "MessageCatchEvent_2"},{"state", "BUSY"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "MessageCatchEvent_2"},{"state", "COMPLETED"}}).size() == 0 );
      }
    }
  }

  GIVEN( "Three instances" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_X; Process_2; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 0\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(0);
      engine.run(std::move(scenario));
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
  const std::string modelFile = "tests/execution/message/Message_tasks.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances without input" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the message is delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "SendTask_1"},{"state", "COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "ReceiveTask_2"},{"state", "COMPLETED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Throwing instance without recipient" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the send task fails" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "SendTask_1"},{"state", "FAILED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Catching instance without sender" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_2; Process_2;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the receive tasks fails" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "ReceiveTask_2"},{"state", "FAILED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Throwing instance waiting for recipient" ) {

    // the recipient becomes known only at its instantiation time, so the sender waits for no known instance
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION; DISCLOSURE\n"
      "Instance_1; Process_1; timestamp := 0; 0\n"
      "Instance_2; Process_2; timestamp := 1; 1\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::DynamicDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Both message tasks fail" ) {
        auto completionLog1 = recorder.find(nlohmann::json{{"processId", "Process_1"},{"state", "FAILED"}});
        REQUIRE( !completionLog1.empty() );
        REQUIRE( completionLog1.back()["status"]["timestamp"] == 0.0 );
        auto completionLog2 = recorder.find(nlohmann::json{{"processId", "Process_2"},{"state", "FAILED"}});
        REQUIRE( !completionLog2.empty() );
        REQUIRE( completionLog2.back()["status"]["timestamp"] == 1.0 );
      }
    }
  }

}

SCENARIO( "Message tasks with timer", "[execution][message]" ) {
  const std::string modelFile = "tests/execution/message/Message_tasks_with_timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances without input" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the message is delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "SendTask_1"},{"state", "COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "ReceiveTask_2"},{"state", "COMPLETED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Throwing instance without recipient" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the message is withdrawn" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_1"},{"instanceId", "Instance_1"},{"nodeId", "SendTask_1"},{"state", "WITHDRAWN"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Catching instance without sender" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_2; Process_2;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the message is withdrawn" ) {
        REQUIRE( recorder.find(nlohmann::json{{"processId", "Process_2"},{"instanceId", "Instance_2"},{"nodeId", "ReceiveTask_2"},{"state", "WITHDRAWN"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Throwing instance waiting for recipient" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 1\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then the message is delivered at time 1" ) {
        auto completionLog1 = recorder.find(nlohmann::json{{"processId", "Process_1"},{"state", "COMPLETED"}});
        REQUIRE( completionLog1.back()["status"]["timestamp"] == 1.0 );
        auto completionLog2 = recorder.find(nlohmann::json{{"processId", "Process_2"},{"state", "COMPLETED"}});
        REQUIRE( completionLog2.back()["status"]["timestamp"] == 1.0 );
      }
    }

  }

}


SCENARIO( "Multi-instance send task", "[execution][message]" ) {
  const std::string modelFile = "tests/execution/message/Multi-instance_send_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One sender with two recipients" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n"
      "Instance_3; Process_2;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then both messages are delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "Activity_1"},{"state", "EXITING"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "Activity_1"},{"state", "DEPARTED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "CatchEvent_2"},{"state", "COMPLETED"}}).size() == 2 );
      }
    }
  }

  GIVEN( "One sender with one recipient" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(1);
      engine.run(std::move(scenario));
      THEN( "Then one message is delivered, but send task is not completed" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "Activity_1"},{"state", "EXITING"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "Activity_1"},{"state", "DEPARTED"}}).size() == 0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "CatchEvent_2"},{"state", "COMPLETED"}}).size() == 1 );
      }
    }
  }

}

SCENARIO( "Multi-instance receive task", "[execution][message]" ) {
  const std::string modelFile = "tests/execution/message/Multi-instance_receive_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One sender with two recipients" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n"
      "Instance_3; Process_2;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      THEN( "Then both messages are delivered and the receive task is completed" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "Activity_1"},{"state", "EXITING"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "Activity_1"},{"state", "DEPARTED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ThrowEvent_2"},{"state", "DEPARTED"}}).size() == 2 );
      }
    }
  }

  GIVEN( "One sender with one recipient" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile);
    auto dataProvider = std::make_shared<Execution::StaticDataProvider>(model, csv);
    auto scenario = dataProvider->createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine(model);
      Execution::InstantEntry entryHandler;
      Execution::FirstMatchingMessageDelivery messageHandler;
      Execution::InstantExit exitHandler;
      messageHandler.connect(&engine);
      entryHandler.connect(&engine);
      exitHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      dataProvider->setEndTime(1);
      engine.run(std::move(scenario));
      THEN( "Then one message is delivered and the receive task is not completed" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "Activity_1"},{"state", "EXITING"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "Activity_1"},{"state", "DEPARTED"}}).size() == 0 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ThrowEvent_2"},{"state", "DEPARTED"}}).size() == 1 );
      }
    }
  }

}

SCENARIO( "Message directed to an instance that ends without receiving it", "[execution][message]" ) {
  const std::string modelFile = "tests/execution/message/Message_to_ending_instance.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  // no dispatcher delivers messages, so the message directed to the recipient stays in its inbox until the
  // recipient ends

  // the position in the log of the first entry matching the given predicate
  auto position = [](const Execution::Recorder& recorder, auto predicate) {
    for ( size_t i = 0; i < recorder.log.size(); i++ ) {
      if ( predicate(recorder.log[i]) ) {
        return i;
      }
    }
    return recorder.log.size();
  };
  auto isWithdrawnMessage = [](const nlohmann::ordered_json& entry) {
    return entry.contains("origin") && entry["origin"] == "MessageEndEvent_1" && entry["state"] == "WITHDRAWN";
  };

  GIVEN( "A recipient completing at time 2" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 0\n"
      "Instance_2; Process_2; duration := 2\n"
      "Instance_2; Process_2; deadline := 10\n"
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
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The message is withdrawn once the recipient is done" ) {
        auto done = position(recorder, [](const nlohmann::ordered_json& entry) {
          return entry.contains("instanceId") && entry["instanceId"] == "Instance_2" && !entry.contains("nodeId") &&
            !entry.contains("event") && !entry.contains("decision") && entry["state"] == "DONE";
        });
        auto withdrawn = position(recorder, isWithdrawnMessage);
        REQUIRE( done < recorder.log.size() );
        REQUIRE( withdrawn < recorder.log.size() );
        REQUIRE( done < withdrawn );
        REQUIRE( engine.getSystemState()->messages.empty() );
        REQUIRE( engine.getSystemState()->stateMachine->tokens.empty() );
      }
    }
  }

  GIVEN( "A recipient missing its deadline at time 2" ) {
    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 0\n"
      "Instance_2; Process_2; duration := 2\n"
      "Instance_2; Process_2; deadline := 1\n"
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
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(dataProvider->createScenario());

      THEN( "The message is withdrawn once the recipient has failed" ) {
        auto failed = position(recorder, [](const nlohmann::ordered_json& entry) {
          return entry.contains("instanceId") && entry["instanceId"] == "Instance_2" && !entry.contains("nodeId") &&
            !entry.contains("event") && !entry.contains("decision") && entry["state"] == "FAILED";
        });
        auto withdrawn = position(recorder, isWithdrawnMessage);
        REQUIRE( failed < recorder.log.size() );
        REQUIRE( withdrawn < recorder.log.size() );
        REQUIRE( failed < withdrawn );
        REQUIRE( engine.getSystemState()->messages.empty() );
        REQUIRE( engine.getSystemState()->stateMachine->tokens.empty() );
      }
    }
  }
}
