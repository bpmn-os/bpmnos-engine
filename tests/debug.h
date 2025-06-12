SCENARIO( "Guided knapsack problem", "[examples][knapsack_problem]" ) {
  const std::string modelFile = "examples/guided_knapsack_problem/Guided_knapsack_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "One knapsack and three items" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "KnapsackProcess;Knapsack1;Items;3\n"
      "KnapsackProcess;Knapsack1;Capacity;40\n"
      "ItemProcess;Item1;Weight;20\n" // ratio = weight / value = 0.2 ( 2nd best )
      "ItemProcess;Item1;Value;100\n"
      "ItemProcess;Item2;Weight;15\n" // ratio = weight / value = 0.3 ( worst )
      "ItemProcess;Item2;Value;50\n"
      "ItemProcess;Item3;Weight;22\n" // ratio = weight / value = 0.183333 ( best )
      "ItemProcess;Item3;Value;120\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    nlohmann::ordered_json log1;
    nlohmann::ordered_json log2;

    WHEN( "The engine is started with the guided controller and greedy decisions" ) {
      Execution::Engine engine1;
      Execution::ReadyHandler readyHandler1;
      Execution::DeterministicTaskCompletion completionHandler1;
      readyHandler1.connect(&engine1);
      completionHandler1.connect(&engine1);

      Execution::GuidedEvaluator evaluator1;
      Execution::GreedyController controller1(&evaluator1, { .bestFirstEntry = false, .bestFirstExit = true });
      controller1.connect(&engine1);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator1;
      Execution::TimeWarp timeHandler1;
      messageTaskTerminator1.connect(&engine1);
      timeHandler1.connect(&engine1);

//      Execution::Recorder recorder;
      Execution::Recorder recorder1(std::cerr); // TODO: FIND OUT WHY THIS PASSES AND BELOW FAILS
      recorder1.subscribe(&engine1);
      engine1.run(scenario.get());
      THEN( "Then the knapsack considers all items" ) {
        auto failureLog = recorder1.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then the knapsack handles items with best value to weight ratio first" ) {
        auto handleLog = recorder1.find(nlohmann::json{{"nodeId", "HandleItemActivity"},{"state", "COMPLETED"}});
        REQUIRE( handleLog[0]["status"]["item"] == "Item3" );
        REQUIRE( handleLog[1]["status"]["item"] == "Item1" );
        REQUIRE( handleLog[2]["status"]["item"] == "Item2" );
      }
      THEN( "Then the knapsack includes Item3 and Item2" ) {
        auto acceptanceLog = recorder1.find(nlohmann::json{{"nodeId", "ItemAccepted"},{"state", "ENTERED"}});
        REQUIRE( (acceptanceLog[0]["instanceId"] == "Item2" || acceptanceLog[0]["instanceId"] == "Item3") );
        REQUIRE( (acceptanceLog[1]["instanceId"] == "Item2" || acceptanceLog[1]["instanceId"] == "Item3") );
      }
      log1 = recorder1.log;
    }

    WHEN( "The engine is started with the guided controller and greedy decisions" ) {
      Execution::Engine engine2;
      Execution::ReadyHandler readyHandler2;
      Execution::DeterministicTaskCompletion completionHandler2;
      readyHandler2.connect(&engine2);
      completionHandler2.connect(&engine2);

      Execution::GuidedEvaluator evaluator2;
      Execution::GreedyController controller2(&evaluator2, { .bestFirstEntry = false, .bestFirstExit = true });
      controller2.connect(&engine2);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator2;
      Execution::TimeWarp timeHandler2;
      messageTaskTerminator2.connect(&engine2);
      timeHandler2.connect(&engine2);

      Execution::Recorder recorder2; // TODO: FIND OUT WHY THIS FAILS AND ABOVE PASSES
//      Execution::Recorder recorder(std::cerr);
      recorder2.subscribe(&engine2);
      engine2.run(scenario.get());

      THEN( "Then the knapsack considers all items" ) {
        auto failureLog = recorder2.find(nlohmann::json{{"nodeId", "SendRequestTask"},{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
      THEN( "Then the knapsack handles items with best value to weight ratio first" ) {
        auto handleLog = recorder2.find(nlohmann::json{{"nodeId", "HandleItemActivity"},{"state", "COMPLETED"}});
        REQUIRE( handleLog[0]["status"]["item"] == "Item3" );
        REQUIRE( handleLog[1]["status"]["item"] == "Item1" );
        REQUIRE( handleLog[2]["status"]["item"] == "Item2" );
      }
      THEN( "Then the knapsack includes Item3 and Item2" ) {
        auto acceptanceLog = recorder2.find(nlohmann::json{{"nodeId", "ItemAccepted"},{"state", "ENTERED"}});
        REQUIRE( (acceptanceLog[0]["instanceId"] == "Item2" || acceptanceLog[0]["instanceId"] == "Item3") );
        REQUIRE( (acceptanceLog[1]["instanceId"] == "Item2" || acceptanceLog[1]["instanceId"] == "Item3") );
      }

      log2 = recorder2.log;
      std::cout << log2 << "\n";
/*
      THEN( "Then the log has the same size" ) {
        REQUIRE( log1.size() == log2.size() );
        for ( unsigned int i = 0; i < std::min(log1.size(),log2.size()); i++ ) {
          std::cerr << log2[i] << "\n";
        }
      }
*/
    }
    
  }
}
