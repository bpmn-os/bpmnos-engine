SCENARIO( "Assignment problem", "[cp][assignment_problem]" ) {
  const std::string modelFile = "examples/assignment_problem/Assignment_problem.bpmn";
  const std::vector<std::string> folders = { "tests/examples/assignment_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile,folders) );

  GIVEN( "Three clients and three servers" ) {

    WHEN( "The engine is started" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "ClientProcess;Client1;;\n"
      "ClientProcess;Client2;;\n"
      "ClientProcess;Client3;;\n"
      "ServerProcess;Server1;;\n"
      "ServerProcess;Server2;;\n"
      "ServerProcess;Server3;;\n"
    ;

      Model::StaticDataProvider dataProvider(modelFile,folders,csv);
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
        REQUIRE( solution.getObjectiveValue().has_value() );
        REQUIRE( solution.getObjectiveValue().value() == engine.getSystemState()->getObjective() );
      }
    }
  }
};

