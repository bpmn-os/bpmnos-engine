SCENARIO( "Loop task - Hexaly solver", "[hexalysolver][loopactivity]" ) {
  const std::string modelFile = "tests/execution/loopactivity/Loop_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    WHEN( "The engine is started with a maximum of 2 loops" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1, Maximum, 2\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::CPModel constraintProgramm( &flattenedGraph );

      // Solve with Hexaly
      const auto& model = constraintProgramm.getModel();
      CP::HexalySolver solver(model);
      auto result = solver.solve(model);

      THEN( "An optimal solution is found" ) {
        REQUIRE( result.has_value() );

        auto& solution = result.value();
        REQUIRE( solution.complete() );
if ( !solution.errors().empty() ) {
  std::cerr << "ERRORS: " << solution.errors() << std::endl;
  std::cerr << "MODEL: " << model.stringify() << std::endl;
  std::cerr << "SOLUTION: " << solution.stringify() << std::endl;
}
        REQUIRE( solution.errors().empty() );
        REQUIRE( solution.getStatus() == CP::Solution::Status::OPTIMAL );
      }
    }
  }
};

SCENARIO( "Loop subprocess - Hexaly solver", "[hexalysolver][loopactivity]" ) {
  const std::string modelFile = "tests/execution/loopactivity/Loop_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    WHEN( "The engine is started with a maximum of 2 loops" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1, Maximum, 2\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::CPModel constraintProgramm( &flattenedGraph );

      // Solve with Hexaly
      const auto& model = constraintProgramm.getModel();
      CP::HexalySolver solver(model);
      auto result = solver.solve(model);

      THEN( "An optimal solution is found" ) {
        REQUIRE( result.has_value() );

        auto& solution = result.value();
        REQUIRE( solution.complete() );
if ( !solution.errors().empty() ) {
  std::cerr << "ERRORS: " << solution.errors() << std::endl;
  std::cerr << "MODEL: " << model.stringify() << std::endl;
  std::cerr << "SOLUTION: " << solution.stringify() << std::endl;
}
        REQUIRE( solution.errors().empty() );
        REQUIRE( solution.getStatus() == CP::Solution::Status::OPTIMAL );
      }
    }
  }
};
