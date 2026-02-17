SCENARIO( "Simple messaging - Hexaly solver", "[hexalysolver][message]" ) {
  const std::string modelFile = "tests/execution/message/Simple_messaging.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "Two instances starting at time 0" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,Timestamp,0\n"
      "Process_2, Instance_2,Timestamp,0\n"
    ;

    WHEN( "Hexaly solver is used" ) {

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

  GIVEN( "Two instances with different ordering" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_2, Instance_2,Timestamp,0\n"
      "Process_1, Instance_1,Timestamp,0\n"
    ;

    WHEN( "Hexaly solver is used" ) {

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


SCENARIO( "Message tasks - Hexaly solver", "[hexalysolver][message]" ) {
  const std::string modelFile = "tests/execution/message/Message_tasks.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Two instances without input" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
      "Process_2, Instance_2,,\n"
    ;

    WHEN( "Hexaly solver is used" ) {

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
