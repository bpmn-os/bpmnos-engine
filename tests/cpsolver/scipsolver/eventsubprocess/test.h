SCENARIO( "N-to-1 assignment - SCIP solver", "[scipsolver][eventsubprocess]" ) {
  const std::string modelFile = "tests/execution/eventsubprocess/N-to-1-assignment.bpmn";

  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A 1-1 assignment" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_2, Instance_1,,\n"
      "Process_1, Instance_0,Expected,1\n"
    ;

    WHEN( "SCIP solver is used" ) {

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::CPModel constraintProgramm( &flattenedGraph );

      // Solve with SCIP
      const auto& model = constraintProgramm.getModel();
      CP::SCIPSolver solver(model);
      auto result = solver.solve(model);

      THEN( "An optimal solution is found" ) {
        REQUIRE( result.has_value() );

        auto& solution = result.value();
        REQUIRE( solution.getStatus() == CP::Solution::Status::OPTIMAL );

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

    WHEN( "SCIP solver is used" ) {
      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::CPModel constraintProgramm( &flattenedGraph );

      // Solve with SCIP
      const auto& model = constraintProgramm.getModel();
      CP::SCIPSolver solver(model);
      auto result = solver.solve(model);

      THEN( "An optimal solution is found" ) {
        REQUIRE( result.has_value() );

        auto& solution = result.value();
        REQUIRE( solution.getStatus() == CP::Solution::Status::OPTIMAL );

        REQUIRE( solution.complete() );
        REQUIRE( solution.errors().empty() );
      }
    }
  }
};
