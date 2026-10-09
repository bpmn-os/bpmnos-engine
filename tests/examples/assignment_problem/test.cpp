#include "prelude.h"
#include "golden/golden.h"

SCENARIO( "Assignment problem", "[examples][assignment_problem]" ) {
  const std::string modelFile = "examples/assignment_problem/Assignment_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/assignment_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "One client and one server" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Client1; ClientProcess;\n"
      "Server1; ServerProcess;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile, folders);
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
      goldenMaster("assignment_problem-1", recorder, engine);
      THEN( "Then the message is delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 1 );
      }
    }
  }

  GIVEN( "Three clients and three servers" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Client1; ClientProcess;\n"
      "Client2; ClientProcess;\n"
      "Client3; ClientProcess;\n"
      "Server1; ServerProcess;\n"
      "Server2; ServerProcess;\n"
      "Server3; ServerProcess;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile, folders);
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
      goldenMaster("assignment_problem-2", recorder, engine);
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
      Execution::Engine engine(model);

      auto evaluator = std::make_shared<Execution::LocalEvaluator>();
      Execution::GreedyController controller(evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      messageTaskTerminator.connect(&engine);

      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(std::move(scenario));
      goldenMaster("assignment_problem-3", recorder, engine);
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
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Client1; ClientProcess;\n"
      "Client2; ClientProcess;\n"
      "Client3; ClientProcess;\n"
      "Server1; ServerProcess;\n"
      "Server2; ServerProcess;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile, folders);
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
      goldenMaster("assignment_problem-4", recorder, engine);
      THEN( "Then one message is not delivered" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "FAILED"}}).size() == 1 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
      }
    }
  }

  GIVEN( "Two clients and three servers" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Client1; ClientProcess;\n"
      "Client2; ClientProcess;\n"
      "Server1; ServerProcess;\n"
      "Server2; ServerProcess;\n"
      "Server3; ServerProcess;\n"
    ;

    auto model = std::make_shared<const Model::Model>(modelFile, folders);
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
      goldenMaster("assignment_problem-5", recorder, engine);
      THEN( "Then one server receives no message" ) {
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "COMPLETED"}}).size() == 2 );
        REQUIRE( recorder.find(nlohmann::json{{"nodeId", "ReceiveRequestTask"},{"state", "FAILED"}}).size() == 1 );
      }
    }
  }

}
