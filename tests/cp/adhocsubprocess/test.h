
SCENARIO( "Sequential adhoc subprocess", "[cpcontroller][adhocsubprocess]" ) {
  const std::string modelFile = "tests/execution/adhocsubprocess/AdHocSubProcess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    WHEN( "The engine is started" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,,\n"
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

SCENARIO( "Sequential adhoc subprocesses with common performer", "[cpcontroller][adhocsubprocess]" ) {
  const std::string modelFile = "tests/execution/adhocsubprocess/AdHocSubProcesses_with_common_performer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    WHEN( "The engine is started" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,,\n"
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

