SCENARIO( "Bin packing problem", "[examples][bin_packing_problem]" ) {
  const std::string modelFile = "examples/bin_packing_problem/Bin_packing_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Three bins and three items" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;Bins;3\n"
      ";;Items;3\n"
      "BinProcess;Bin1;Capacity;40.0\n"
      "BinProcess;Bin2;Capacity;40.0\n"
      "BinProcess;Bin3;Capacity;40.0\n"
      "ItemProcess;Item1;Size;20.0\n"
      "ItemProcess;Item2;Size;15.0\n"
      "ItemProcess;Item3;Size;22.0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      readyHandler.connect(&engine);
      completionHandler.connect(&engine);

      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      THEN( "Then no failure occurs" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then all wait activities are completed" ) {
        auto log = recorder.find(nlohmann::json{{"nodeId","WaitActivity"}, {"state", "COMPLETED"} }, nlohmann::json{{"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.size() << ": " << log.dump() << std::endl;
        REQUIRE( log.size() == 3 );
      }      
      THEN( "Then all bin process instances come to the end" ) {
        auto log = recorder.find(nlohmann::json{{"nodeId","EndEventBin"}, {"state", "DONE"} }, nlohmann::json{{"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.size() << ": " << log.dump() << std::endl;
        REQUIRE( log.size() == 3 );
      }      
      
      THEN( "Then all bin process instances complete" ) {
        auto log = recorder.find({{"processId","BinProcess" },{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.dump() << std::endl;
        REQUIRE( log.size() == 3 );
      }
      THEN( "Then all item process instances complete" ) {
        auto itemProcessLog = recorder.find({{"processId","ItemProcess" },{"state","DONE"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr }, {"decision",nullptr }});
//std::cerr << itemProcessLog.dump() << std::endl;
        REQUIRE( itemProcessLog.size() == 3 );
      }
    }
  }

  GIVEN( "Four bins and four items" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;Bins;4\n"
      ";;Items;4\n"
      "BinProcess;Bin1;Capacity;100.0\n"
      "BinProcess;Bin2;Capacity;100.0\n"
      "BinProcess;Bin3;Capacity;100.0\n"
      "BinProcess;Bin4;Capacity;100.0\n"
      "ItemProcess;Item1;Size;36.6\n"
      "ItemProcess;Item2;Size;26.8\n"
      "ItemProcess;Item3;Size;36.6\n"
      "ItemProcess;Item4;Size;43.0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      readyHandler.connect(&engine);
      completionHandler.connect(&engine);

      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      THEN( "Then no failure occurs" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then all wait activities are completed" ) {
        auto log = recorder.find(nlohmann::json{{"nodeId","WaitActivity"}, {"state", "COMPLETED"} }, nlohmann::json{{"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.size() << ": " << log.dump() << std::endl;
        REQUIRE( log.size() == 4 );
      }      
      THEN( "Then all bin process instances come to the end" ) {
        auto log = recorder.find(nlohmann::json{{"nodeId","EndEventBin"}, {"state", "DONE"} }, nlohmann::json{{"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.size() << ": " << log.dump() << std::endl;
        REQUIRE( log.size() == 4 );
      }      
      
      THEN( "Then all bin process instances complete" ) {
        auto log = recorder.find({{"processId","BinProcess" },{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.dump() << std::endl;
        REQUIRE( log.size() == 4 );
      }
      THEN( "Then all item process instances complete" ) {
        auto itemProcessLog = recorder.find({{"processId","ItemProcess" },{"state","DONE"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr }, {"decision",nullptr }});
//std::cerr << itemProcessLog.dump() << std::endl;
        REQUIRE( itemProcessLog.size() == 4 );
      }
    }
  }
}

