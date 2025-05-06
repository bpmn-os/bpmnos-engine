SCENARIO( "Task with linear expression incrementing timestamp", "[cpcontroller][task]" ) {
  const std::string modelFile = "tests/execution/task/Task_with_linear_expression.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;


    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

    WHEN( "The model is created" ) {
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
//      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); // only necessary to validate consistency of solution and identify errors
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      auto& solution = controller.createSolution();
      solution.subscribe(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      
//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
      THEN( "The solution is complete and satisfies all constraints" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );  
        REQUIRE( solution.complete() ); // requires subscription of controller to engine
        REQUIRE( solution.errors().empty() ); // requires subscription of controller to engine
      }
      THEN( "The dump of each entry of the token log is correct" ) {
        auto activityLog = recorder.find(nlohmann::json{{"nodeId","Activity_1" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
//std::cerr << "Log:\n" << activityLog.dump() << std::endl;
        REQUIRE( activityLog[0]["state"] == "ARRIVED" );
        REQUIRE( activityLog[1]["state"] == "READY" );
        REQUIRE( activityLog[2]["state"] == "ENTERED" );
        REQUIRE( activityLog[2]["status"]["timestamp"] == 0.0);
        REQUIRE( activityLog[3]["state"] == "BUSY" );
        REQUIRE( activityLog[3]["status"]["timestamp"] == 1.0);
        REQUIRE( activityLog[4]["state"] == "COMPLETED" );
        REQUIRE( activityLog[4]["status"]["timestamp"] == 1.0);
        REQUIRE( activityLog[5]["state"] == "EXITING" );
        REQUIRE( activityLog[6]["state"] == "DEPARTED" );

        REQUIRE( activityLog.back()["instanceId"] == "Instance_1");
        REQUIRE( activityLog.back()["processId"] == "Process_1");
        REQUIRE( activityLog.back()["state"] == "DEPARTED");
        REQUIRE( activityLog.back()["data"]["instance"] == "Instance_1");
        REQUIRE( activityLog.back()["status"]["timestamp"] == 1.0);
      }
      

    }
  }
};


