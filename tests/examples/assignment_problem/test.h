SCENARIO( "Assignment problem", "[examples][assignment_problem]" ) {
  const std::string modelFile = "examples/assignment_problem/Assignment_problem.bpmn";
  BPMNOS::Model::LookupTable::folders = { "examples/assignment_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One client and one server" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "ClientProcess;Client1;;\n"
      "ServerProcess;Server1;;\n"
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
      messageHandler.connect(&engine);
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the message is delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Three clients and three servers" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "ClientProcess;Client1;;\n"
      "ClientProcess;Client2;;\n"
      "ClientProcess;Client3;;\n"
      "ServerProcess;Server1;;\n"
      "ServerProcess;Server2;;\n"
      "ServerProcess;Server3;;\n"
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
      messageHandler.connect(&engine);
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the messages are delivered in any order" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 3 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 3 );
        
/*
        auto assignmentLog = recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}});
        REQUIRE( assignmentLog[0]["data"]["instance"] == "Server1" );
        REQUIRE( assignmentLog[0]["status"]["client"] == "Client1" );

        REQUIRE( assignmentLog[1]["data"]["instance"] == "Server2" );
        REQUIRE( assignmentLog[1]["status"]["client"] == "Client2" );

        REQUIRE( assignmentLog[2]["data"]["instance"] == "Server3" );
        REQUIRE( assignmentLog[2]["status"]["client"] == "Client3" );
*/
      }
    }

    WHEN( "The engine is started with the greedy controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      readyHandler.connect(&engine);
      completionHandler.connect(&engine);

      Execution::LocalEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the messages are delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 3 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 3 );

        auto assignmentLog = recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}});
        REQUIRE( assignmentLog[0]["data"]["instance"] == "Server2" );
        REQUIRE( assignmentLog[0]["status"]["client"] == "Client3" );

        REQUIRE( assignmentLog[1]["data"]["instance"] == "Server1" );
        REQUIRE( assignmentLog[1]["status"]["client"] == "Client2" );

        REQUIRE( assignmentLog[2]["data"]["instance"] == "Server3" );
        REQUIRE( assignmentLog[2]["status"]["client"] == "Client1" );
      }
    }
  }

  GIVEN( "Three clients and two servers" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "ClientProcess;Client1;;\n"
      "ClientProcess;Client2;;\n"
      "ClientProcess;Client3;;\n"
      "ServerProcess;Server1;;\n"
      "ServerProcess;Server2;;\n"
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
      messageHandler.connect(&engine);
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then one message is not delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "FAILED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
      }
    }
  }

  GIVEN( "Two clients and three servers" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "ClientProcess;Client1;;\n"
      "ClientProcess;Client2;;\n"
      "ServerProcess;Server1;;\n"
      "ServerProcess;Server2;;\n"
      "ServerProcess;Server3;;\n"
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
      messageHandler.connect(&engine);
      readyHandler.connect(&engine);
      entryHandler.connect(&engine);
      completionHandler.connect(&engine);
      exitHandler.connect(&engine);
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then one server receives no message" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "FAILED"}}).size() == 1 );
      }
    }
  }

}
