SCENARIO( "N-to-1 assignment", "[cpcontroller][eventsubprocess]" ) {
  const std::string modelFile = "tests/execution/eventsubprocess/N-to-1-assignment.bpmn";

  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A 1-1 assignment" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_2, Instance_1,,\n"
      "Process_1, Instance_0,Expected,1\n"
    ;

    WHEN( "The engine is started" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
 
      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); 
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);

//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors:\n" << solution.errors() << std::endl;
      THEN( "The solution is complete and satisfies all constraints" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );  
        REQUIRE( solution.complete() );
        REQUIRE( solution.errors().empty() );
      }
    }
  }
 
  GIVEN( "A 2-1 assignment" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_2, Instance_1,,\n"
      "Process_2, Instance_2,,\n"
      "Process_1, Instance_3,Expected,2\n"
    ;

    WHEN( "The engine is started" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
 
      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( {1,2,3,4,5,6,7,8,9,10,11,12,13,14,16,15,17,19,22,20,23,25,27,18,21,24,26,28} );

      auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); 
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);

//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors:\n" << solution.errors() << std::endl;
      THEN( "The solution is complete and satisfies all constraints" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );  
        REQUIRE( solution.complete() );
        REQUIRE( solution.errors().empty() );
      }
    }
  }
};

