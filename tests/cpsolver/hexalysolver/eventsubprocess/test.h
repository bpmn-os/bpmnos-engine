SCENARIO( "N-to-1 assignment - Hexaly solver", "[hexalysolver][eventsubprocess]" ) {
  const std::string modelFile = "tests/execution/eventsubprocess/N-to-1-assignment.bpmn";

  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A 1-1 assignment" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_2;\n"
      "Instance_0; Process_1; expected := 1\n"
    ;

    WHEN( "Hexaly solver is used" ) {

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

  GIVEN( "A 2-1 assignment" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_2;\n"
      "Instance_2; Process_2;\n"
      "Instance_3; Process_1; expected := 2\n"
    ;

    WHEN( "Hexaly solver is used" ) {
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
