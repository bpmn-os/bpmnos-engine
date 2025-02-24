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
      controller.subscribe(&engine); // only necessary to validate consistency of solution and identify errors
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      
//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
      REQUIRE( solution.complete() ); // requires subscription of controller to engine
      REQUIRE( solution.errors().empty() ); // requires subscription of controller to engine
      

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
      controller.subscribe(&engine); // only necessary to validate consistency of solution and identify errors
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      
//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
      REQUIRE( solution.complete() ); // requires subscription of controller to engine
      REQUIRE( solution.errors().empty() ); // requires subscription of controller to engine
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
      controller.subscribe(&engine); // only necessary to validate consistency of solution and identify errors
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
      Execution::Recorder recorder;
//      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      
//std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
//std::cerr << "Solution:\n" << solution.stringify() << std::endl;
      REQUIRE( solution.complete() ); // requires subscription of controller to engine
      REQUIRE( solution.errors().empty() ); // requires subscription of controller to engine
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

    WHEN( "The model is created" ) {
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller(scenario.get(), &evaluator);
      controller.setSeed( Execution::CPSeed::defaultSeed( controller.getVertices().size() ) );

      auto& solution = controller.createSolution();
      Execution::Engine engine;
      controller.connect(&engine);
      controller.subscribe(&engine); // only necessary to validate consistency of solution and identify errors
      Execution::TimeWarp timeHandler;
      timeHandler.connect(&engine);
//      Execution::Recorder recorder;
      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get(),10);
      
std::cerr << "Model:\n" << controller.getModel().stringify() << std::endl;
std::cerr << "Solution:\n" << solution.stringify() << std::endl;
      REQUIRE( solution.complete() ); // requires subscription of controller to engine
      REQUIRE( solution.errors().empty() ); // requires subscription of controller to engine
    }
  }

};

