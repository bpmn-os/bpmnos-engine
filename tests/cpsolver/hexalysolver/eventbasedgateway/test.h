SCENARIO( "Event-based gateway with two timer events - Hexaly solver", "[hexalysolver][eventbasedgateway]" ) {
  const std::string modelFile = "tests/execution/eventbasedgateway/Two_timer_events.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "Timer 1 triggers before Timer 2" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Trigger1,1\n"
        "Process_1, Instance_1,Trigger2,2\n"
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
        REQUIRE( solution.getStatus() == CP::Solution::Status::OPTIMAL );

        REQUIRE( solution.complete() );
if ( !solution.errors().empty() ) {
  std::cerr << "ERRORS: " << solution.errors() << std::endl;
  std::cerr << "MODEL: " << model.stringify() << std::endl;
  std::cerr << "SOLUTION: " << solution.stringify() << std::endl;
}
        REQUIRE( solution.errors().empty() );
      }
    }

    WHEN( "Timer 1 triggers after Timer 2" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Trigger1,3\n"
        "Process_1, Instance_1,Trigger2,2\n"
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
        REQUIRE( solution.getStatus() == CP::Solution::Status::OPTIMAL );
        REQUIRE( solution.complete() );
if ( !solution.errors().empty() ) {
  std::cerr << "ERRORS: " << solution.errors() << std::endl;
  std::cerr << "MODEL: " << model.stringify() << std::endl;
  std::cerr << "SOLUTION: " << solution.stringify() << std::endl;
}
        REQUIRE( solution.errors().empty() );
      }
    }
  }
};
