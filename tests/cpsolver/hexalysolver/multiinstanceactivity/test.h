SCENARIO( "Parallel multi instance task - Hexaly solver", "[hexalysolver][multiinstanceactivity]" ) {
  const std::string modelFile = "tests/execution/multiinstanceactivity/Parallel_multi-instance_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    WHEN( "Hexaly solver is used" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,,\n"
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

        // Debug: print errors
        const auto& errors = solution.errors();
        if (!errors.empty()) {
          std::cerr << "Errors found: " << errors << std::endl;
        }

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


SCENARIO( "Sequential multi instance task - Hexaly solver", "[hexalysolver][multiinstanceactivity]" ) {
  const std::string modelFile = "tests/execution/multiinstanceactivity/Sequential_multi-instance_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    WHEN( "Hexaly solver is used" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,,\n"
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

        // Debug: print errors
        const auto& errors = solution.errors();

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
