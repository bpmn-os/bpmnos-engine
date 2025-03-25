SCENARIO( "Task with expresion operator", "[cpcontroller][data]" ) {
  const std::string modelFile = "tests/execution/data/Data.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with an input value" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,DataAttribute_1,8\n"
//      "Process_1, Instance_1,DataAttribute_2,15\n"
    ;


    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

    WHEN( "The model is created" ) {
      Execution::GuidedEvaluator evaluator;
//std::cerr << "SeededGreedyController()" << std::endl;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); // only necessary to validate consistency of solution and identify errors

//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;

      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      
//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
      THEN( "The solution is complete and satisfies all constraints" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );  
        REQUIRE( solution.complete() ); // requires subscription of controller to engine
        REQUIRE( solution.errors().empty() ); // requires subscription of controller to engine
      }
    }
  }
};


