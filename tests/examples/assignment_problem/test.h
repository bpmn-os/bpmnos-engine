SCENARIO( "Assignment problem", "[examples][assignment_problem]" ) {
  const std::string modelFile = "examples/assignment_problem/Assignment_problem.bpmn";
  BPMNOS::Model::LookupTable::folders = { std::string(std::filesystem::current_path()) + "/examples/assignment_problem" };
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
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::FirstMatchingMessageHandler messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExitHandler exitHandler;
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
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::FirstMatchingMessageHandler messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExitHandler exitHandler;
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
      THEN( "Then the messages are delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 3 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 3 );
      }
    }
  }

  GIVEN( "Three clients and two servers" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "ClientProcess;lient1;;\n"
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
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::FirstMatchingMessageHandler messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExitHandler exitHandler;
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
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::FirstMatchingMessageHandler messageHandler;
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::InstantExitHandler exitHandler;
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
      THEN( "Then one server receives no message" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "FAILED"}}).size() == 1 );
      }
    }
  }

}
