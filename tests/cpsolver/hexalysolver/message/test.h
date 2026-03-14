SCENARIO( "Simple messaging - Hexaly solver", "[hexalysolver][message]" ) {
  const std::string modelFile = "tests/execution/message/Simple_messaging.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "Two instances starting at time 0" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1; timestamp := 0\n"
      "Instance_2; Process_2; timestamp := 0\n"
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

  GIVEN( "Two instances with different ordering" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_2; Process_2; timestamp := 0\n"
      "Instance_1; Process_1; timestamp := 0\n"
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


SCENARIO( "Message tasks - Hexaly solver", "[hexalysolver][message]" ) {
  const std::string modelFile = "tests/execution/message/Message_tasks.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances without input" ) {

    std::string csv =
      "INSTANCE_ID; NODE_ID; INITIALIZATION\n"
      "Instance_1; Process_1;\n"
      "Instance_2; Process_2;\n"
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
