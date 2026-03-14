SCENARIO( "Decision task with enumeration - SCIP solver", "[scipsolver][decisiontask]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_enumeration.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with x=-2" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_1; Activity_1; x := -2\n"
    ;

    WHEN( "SCIP solver is used" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::CPModel constraintProgramm( &flattenedGraph );

      // Solve with SCIP
      const auto& model = constraintProgramm.getModel();
      CP::SCIPSolver solver(model);
      auto result = solver.solve();

      THEN( "An optimal solution is found" ) {
        REQUIRE( result.status == CP::Solver::Result::SOLUTION::OPTIMAL );

        auto solution = solver.getSolution();

        REQUIRE( solution->complete() );
if ( !solution->errors().empty() ) {
  std::cerr << "ERRORS: " << solution->errors() << std::endl;
  std::cerr << "MODEL: " << model.stringify() << std::endl;
  std::cerr << "SOLUTION: " << solution->stringify() << std::endl;
}
        REQUIRE( solution->errors().empty() );
      }
    }
  }
};


SCENARIO( "Decision task with bounds - SCIP solver", "[scipsolver][decisiontask]" ) {
  const std::string modelFile = "tests/execution/decisiontask/DecisionTask_with_bounds.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
    ;

    WHEN( "SCIP solver is used" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::CPModel constraintProgramm( &flattenedGraph );

      // Solve with SCIP
      const auto& model = constraintProgramm.getModel();
      CP::SCIPSolver solver(model);
      auto result = solver.solve();

      THEN( "An optimal solution is found" ) {
        REQUIRE( result.status == CP::Solver::Result::SOLUTION::OPTIMAL );

        auto solution = solver.getSolution();

        REQUIRE( solution->complete() );
        REQUIRE( solution->errors().empty() );
      }
    }
  }
};
