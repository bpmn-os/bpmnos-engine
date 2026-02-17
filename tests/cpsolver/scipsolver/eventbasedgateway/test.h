SCENARIO( "Event-based gateway with two timer events - SCIP solver", "[scipsolver][eventbasedgateway]" ) {
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

      // Solve with SCIP
      const auto& model = constraintProgramm.getModel();
      CP::SCIPSolver solver(model);
      auto result = solver.solve(model);

      THEN( "An optimal solution is found" ) {
        REQUIRE( result.has_value() );

        auto& solution = result.value();
        REQUIRE( solution.getStatus() == CP::Solution::Status::OPTIMAL );

        REQUIRE( solution.complete() );
if ( !solution.errors().empty() ) {
  std::cerr << solution.errors() << std::endl;
  std::cerr << solution.stringify() << std::endl;
  std::cerr << model.stringify() << std::endl;
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

      // Solve with SCIP
      const auto& model = constraintProgramm.getModel();
      CP::SCIPSolver solver(model);

      SCIP* scip = solver.getScip();
      SCIPsetIntParam(scip, "presolving/maxrounds", 2);
      WARN("SCIP presolve maxrounds set to 2, https://github.com/scipopt/scip/issues/190"); 
      
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
