SCENARIO( "Bin packing problem", "[examples][bin_packing_problem]" ) {
  const std::string modelFile = "examples/bin_packing_problem/Bin_packing_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Three bins and three items" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; bins := 3\n"
      "; ; items := 3\n"
      "Bin1; BinProcess; capacity := 40.0\n"
      "Bin2; BinProcess; capacity := 40.0\n"
      "Bin3; BinProcess; capacity := 40.0\n"
      "Item1; ItemProcess; size := 20.0\n"
      "Item2; ItemProcess; size := 15.0\n"
      "Item3; ItemProcess; size := 22.0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;

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
      engine.run(scenario.get(),2); // TODO: time limit should be removed when strange error below is fixed

      THEN( "Then no failure occurs" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then all wait activities are completed" ) {
        auto waitLog = recorder.find(nlohmann::json{{"nodeId","WaitActivity"}, {"state", "COMPLETED"} }, nlohmann::json{{"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.size() << ": " << log.dump() << std::endl;
        REQUIRE( waitLog.size() == 3 );
      }      
      THEN( "Then all bin process instances come to the end" ) {
        auto endEventLog = recorder.find(nlohmann::json{{"nodeId","EndEventBin"}, {"state", "DONE"} }, nlohmann::json{{"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.size() << ": " << log.dump() << std::endl;
        REQUIRE( endEventLog.size() == 3 );
      }      
      THEN( "Then all bin process instances complete" ) {
        auto binLog = recorder.find({{"processId","BinProcess" },{"state","COMPLETED"}}, nlohmann::json{{"nodeId",nullptr }, {"event",nullptr }, {"decision",nullptr }});
//std::cerr << log.dump() << std::endl;
        REQUIRE( binLog.size() == 3 );
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
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "; ; bins := 4\n"
      "; ; items := 4\n"
      "Bin1; BinProcess; capacity := 100.0\n"
      "Bin2; BinProcess; capacity := 100.0\n"
      "Bin3; BinProcess; capacity := 100.0\n"
      "Bin4; BinProcess; capacity := 100.0\n"
      "Item1; ItemProcess; size := 36.6\n"
      "Item2; ItemProcess; size := 26.8\n"
      "Item3; ItemProcess; size := 36.6\n"
      "Item4; ItemProcess; size := 43.0\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;

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

