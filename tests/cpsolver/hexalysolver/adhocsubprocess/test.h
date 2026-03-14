SCENARIO( "Sequential adhoc subprocess - Hexaly solver", "[hexalysolver][adhocsubprocess]" ) {
  const std::string modelFile = "tests/execution/adhocsubprocess/AdHocSubProcess.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    WHEN( "Hexaly solver is used" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::CPModel constraintProgramm( &flattenedGraph );

      // Solve with Hexaly
      const auto& model = constraintProgramm.getModel();
      CP::HexalySolver solver(model);
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

SCENARIO( "Sequential adhoc subprocesses with common performer - Hexaly solver", "[hexalysolver][adhocsubprocess]" ) {
  const std::string modelFile = "tests/execution/adhocsubprocess/AdHocSubProcesses_with_common_performer.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    WHEN( "Hexaly solver is used" ) {
      std::string csv =
        "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
        "Instance_1; Process_1;\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::CPModel constraintProgramm( &flattenedGraph );

      // Solve with Hexaly
      const auto& model = constraintProgramm.getModel();
      CP::HexalySolver solver(model);
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
