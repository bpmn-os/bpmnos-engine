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

    REQUIRE_NOTHROW( BPMNOS::Execution::FlattenedGraph(scenario.get()) );

    WHEN( "The model is created" ) {
      Execution::CPController controller(scenario.get(), {.instantEntry = true,.instantExit = true});
      auto& cpmodel = controller.getModel();
      auto cp = cpmodel.stringify();
      std::cout << cp << std::endl;
/*
///////
      auto& cpmodel = controller.createCP(scenario.get());
      auto cp = cpmodel.stringify();
      
      std::cout << cp << std::endl;
      
      THEN( "The model has the right variables" ) {
        REQUIRE( cp.find("x_{Instance_1,Process_1} := 1.00") != std::string::npos );
        REQUIRE( cp.find("y^{entry,0,Process_1}_{Instance_1,Process_1} := 1.00") != std::string::npos );
        REQUIRE( cp.find("y^{exit,0,Process_1}_{Instance_1,Process_1} ∈ [ 0.00, 1.00 ]") != std::string::npos );     
        
        REQUIRE( cp.find("0.00 + 1.00*y^{entry,0,Process_1}_{Instance_1,Process_1} - 1.00*x_{Instance_1,Process_1} == 0") != std::string::npos );
        REQUIRE( cp.find("0.00 + 1.00*y^{exit,0,Process_1}_{Instance_1,Process_1} - 1.00*y^{entry,0,Process_1}_{Instance_1,Process_1} <= 0") != std::string::npos );
        REQUIRE( cp.find("0.00 + 1.00*y^{exit,0,Process_1}_{Instance_1,Process_1} - 1.00*x_{Instance_1,Process_1} == 0") != std::string::npos );

        REQUIRE( cp.find("defined^entry_{Instance_1,Process_1,instance} := 1.00") != std::string::npos );
        REQUIRE( cp.find("value^entry_{Instance_1,Process_1,instance} := 2.00") != std::string::npos );
        
        REQUIRE( cp.find("defined^0_{Instance_1,Process_1} := 0.00 + 1.00*defined^entry_{Instance_1,Process_1,instance}") != std::string::npos );
        REQUIRE( cp.find("value^0_{Instance_1,Process_1} := 0.00 + 1.00*value^entry_{Instance_1,Process_1,instance}") != std::string::npos );

        REQUIRE( cp.find("defined^exit_[Instance_1,Process_1,instance} := if y^{exit,0,Process_1}_{Instance_1,Process_1} then 0.00 + 1.00*defined^0_{Instance_1,Process_1} else 0.00") != std::string::npos );
        REQUIRE( cp.find("value^exit_{Instance_1,Process_1,instance} := if y^{exit,0,Process_1}_{Instance_1,Process_1} then 0.00 + 1.00*value^0_{Instance_1,Process_1} else 0.00") != std::string::npos );
        
        REQUIRE( cp.find("defined^entry_{Instance_1,Process_1,timestamp} := 1.00") != std::string::npos );
        REQUIRE( cp.find("value^entry_{Instance_1,Process_1,timestamp} := 0.00") != std::string::npos );

        REQUIRE( cp.find("defined^exit_[Instance_1,Process_1,timestamp} := 0.00 + 1.00*defined^entry_{Instance_1,Process_1,timestamp}") != std::string::npos );
        REQUIRE( cp.find("value^exit_{Instance_1,Process_1,timestamp} := 0.00 + 1.00*value^entry_{Instance_1,Process_1,timestamp}") != std::string::npos );
      }
*/
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
      Execution::CPController controller(scenario.get());
      auto& cpmodel = controller.getModel();
//      std::cout << cpmodel.stringify() << std::endl;
//      auto cp = cpmodel.stringify();

//      std::cout << cp << std::endl;
    }
  }
};

