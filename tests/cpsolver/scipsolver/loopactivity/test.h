SCENARIO( "Loop task - SCIP solver", "[scipsolver][loopactivity]" ) {
  const std::string modelFile = "tests/execution/loopactivity/Loop_task.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    WHEN( "The engine is started with a maximum of 2 loops" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
        "Instance_1; LoopActivity_1; maximum := 2\n"
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

SCENARIO( "Loop subprocess - SCIP solver", "[scipsolver][loopactivity]" ) {
  const std::string modelFile = "tests/execution/loopactivity/Loop_subprocess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    WHEN( "The engine is started with a maximum of 2 loops" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
        "Instance_1; LoopActivity_1; maximum := 2\n"
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
