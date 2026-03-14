SCENARIO( "Simple process with timer - SCIP solver", "[scipsolver][timer]" ) {
  const std::string modelFile = "tests/execution/timer/Timer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance" ) {
    WHEN( "SCIP solver is used with a given trigger" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1; trigger := 10\n"
      ;

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
