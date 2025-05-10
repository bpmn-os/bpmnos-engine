SCENARIO( "Symmetric exclusivegateway gateways", "[cpcontroller][exclusivegateway]" ) {
  const std::string modelFile = "tests/execution/exclusivegateway/Symmetric.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "The engine is started starting at time 0" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Timestamp,0\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
 
      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller( &flattenedGraph, &evaluator );

      Execution::Engine engine;
      controller.connect( &engine );
      controller.subscribe( &engine );
      Execution::TimeWarp timeHandler;
      timeHandler.connect( &engine );
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe( &engine );

      Execution::CPModel constraintProgramm( &flattenedGraph );
      Execution::CPSolution solution( &constraintProgramm );
      solution.subscribe( &engine );

      engine.run(scenario.get());
      
//std::cerr << "Model:\n" << constraintProgramm.stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors: " << solution.errors() << std::endl;

      THEN( "The solution is complete and satisfies all constraints" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );
        REQUIRE( solution.complete() );
        REQUIRE( solution.errors().empty() );
      }
    }

    WHEN( "The engine is started at time 2" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Timestamp,2\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();
 
      REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller( &flattenedGraph, &evaluator );

      Execution::Engine engine;
      controller.connect( &engine );
      controller.subscribe( &engine );
      Execution::TimeWarp timeHandler;
      timeHandler.connect( &engine );
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe( &engine );

      Execution::CPModel constraintProgramm( &flattenedGraph );
      Execution::CPSolution solution( &constraintProgramm );
      solution.subscribe( &engine );

      engine.run(scenario.get());
      
//std::cerr << "Model:\n" << constraintProgramm.stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors: " << solution.errors() << std::endl;

      THEN( "The solution is complete and satisfies all constraints" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );
        REQUIRE( solution.complete() );
        REQUIRE( solution.errors().empty() );
      }
    }  
  }
};

