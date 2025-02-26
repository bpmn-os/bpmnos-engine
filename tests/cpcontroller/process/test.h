#include <sstream>

SCENARIO( "Empty executable process", "[cpcontroller][process]" ) {
  const std::string modelFile = "tests/execution/process/Empty_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    REQUIRE_NOTHROW( Execution::FlattenedGraph(scenario.get()) );

    WHEN( "The model is created" ) {
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine);
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
        REQUIRE( solution.complete() );
        REQUIRE( solution.errors().empty() );
      }
    }
  }
};

SCENARIO( "Trivial executable process", "[cpcontroller][process]" ) {
  const std::string modelFile = "tests/execution/process/Trivial_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    REQUIRE_NOTHROW( BPMNOS::Execution::FlattenedGraph(scenario.get()) );

    WHEN( "The model is created" ) {
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine);
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
        REQUIRE( solution.complete() ); 
        REQUIRE( solution.errors().empty() ); 
      }
    }
  }
};

SCENARIO( "Simple executable process", "[cpcontroller][process]" ) {
  const std::string modelFile = "tests/execution/process/Constrained_executable_process.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A single instance with no input values" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    REQUIRE_NOTHROW( BPMNOS::Execution::FlattenedGraph(scenario.get()) );

    WHEN( "The model is created" ) {
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine);
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
        REQUIRE( solution.complete() ); 
        REQUIRE( solution.errors().empty() );
      }
    }
  }

  GIVEN( "Two instances with no input values" ) {
    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
      "Process_1, Instance_2,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    REQUIRE_NOTHROW( BPMNOS::Execution::FlattenedGraph(scenario.get()) );

    WHEN( "The model is solved with a suitable seed" ) {
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( {1,9,2,10,3,11,4,12,5,13,6,14,7,15,8,16} );

      auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine);
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      
//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors: " << solution.errors() << std::endl;
      THEN( "The solution is complete and satisfies all constraints" ) {
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( terminationLog.empty() );  
        REQUIRE( solution.complete() ); 
        REQUIRE( solution.errors().empty() );
      }
    }

    WHEN( "The model is solved with the default seed" ) {
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);

      auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine);
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      
//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
//std::cerr << "Errors: " << solution.errors() << std::endl;
      THEN( "The solution is infeasible and does not satisfy all constraints" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state","FAILED" }}, nlohmann::json{{"event",nullptr },{"decision",nullptr }});
        REQUIRE( !failureLog.empty() ); 
        auto terminationLog = recorder.find(nlohmann::json{{"event","termination"}});
        REQUIRE( !terminationLog.empty() );  
        REQUIRE( !solution.errors().empty() ); 
      }
    }
  }

};

