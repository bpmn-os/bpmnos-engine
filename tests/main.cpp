#include <catch2/catch_test_macros.hpp>
#define CATCH_CONFIG_NO_THROW
#include <bpmn++.h>
#include "bpmnos-model.h"
#include "bpmnos-execution.h"
#include <iostream>

using namespace BPMNOS;

// Include all tests here

#define ALL_TESTS
#ifdef ALL_TESTS

/* Model */
#include "model/parser/test.h"
/* Data provider */
#include "data/static/test.h"
//#include "data/dynamic/test.h"
/* Execution engine */
// Process
#include "execution/process/test.h"

// Activities
#include "execution/task/test.h"
#include "execution/subprocess/test.h"
#include "execution/decisiontask/test.h"
//#include "execution/request/test.h" // TODO: Check allocation id in content of request message

// Expression
#include "execution/expression/test.h"

// Gateways
#include "execution/parallelgateway/test.h"
#include "execution/exclusivegateway/test.h"

// Event-based gateways
#include "execution/eventbasedgateway/test.h"
// Events
#include "execution/timer/test.h"
#include "execution/signal/test.h"
#include "execution/condition/test.h"
#include "execution/errorevent/test.h"
#include "execution/escalationevent/test.h"
// Messages
#include "execution/message/test.h"
// Boundary events
#include "execution/boundaryevent/test.h"

// Event subprocesses
#include "execution/eventsubprocess/test.h"

// Compensations
#include "execution/compensationactivity/test.h"
#include "execution/compensationeventsubprocess/test.h"
// Multi-instance activities
#include "execution/loopactivity/test.h"
#include "execution/multiinstanceactivity/test.h"
// Ad-hoc subprocesses
#include "execution/adhocsubprocess/test.h"

// Status and Data
#include "execution/status/test.h"
#include "execution/data/test.h"
#include "execution/collection/test.h"

// Examples
#include "examples/bin_packing_problem/test.h" 
//#include "examples/guided_bin_packing_problem/test.h" // TODO
#include "examples/travelling_salesperson_problem/test.h"
#include "examples/assignment_problem/test.h"
#include "examples/job_shop_scheduling_problem/test.h"
#include "examples/knapsack_problem/test.h"
#include "examples/guided_knapsack_problem/test.h"
#include "examples/vehicle_routing_problem/test.h"
//#include "examples/guided_vehicle_routing_problem/test.h" // TODO
#include "examples/pickup_delivery_problem/test.h"
//#include "examples/guided_pickup_delivery_problem/test.h" // TODO
#include "examples/truck_driver_scheduling_problem/test.h"

// CPController
#include "cpmodel/test.h"
#include "cpsolver/test.h"

#endif // ALL_TESTS

#ifndef ALL_TESTS
#include <cp/cp.h>
#include <cp/solver.h>
#include <cp/scip/scip_adapter.h>

SCENARIO( "Event-based gateway with two timer events - SCIP solver", "[cpsolver][eventbasedgateway]" ) {
  const std::string modelFile = "tests/execution/eventbasedgateway/Two_timer_events.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance" ) {

    WHEN( "Timer 1 triggers after Timer 2" ) {
      std::string csv =
        "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
        "Process_1, Instance_1,Trigger1,3\n"
        "Process_1, Instance_1,Trigger2,2\n"
      ;

      Model::StaticDataProvider dataProvider(modelFile,csv);
      auto scenario = dataProvider.createScenario();

      // First, run through execution engine to get feasible solution
      Execution::FlattenedGraph flattenedGraph( scenario.get() );
      Execution::GuidedEvaluator evaluator;
      Execution::SeededGreedyController controller( &flattenedGraph, &evaluator );

      Execution::Engine engine;
      controller.connect( &engine );
      controller.subscribe( &engine );
      Execution::TimeWarp timeHandler;
      timeHandler.connect( &engine );

      Execution::CPModel constraintProgramm( &flattenedGraph );
      Execution::CPSolution cpSolution( &constraintProgramm );
      cpSolution.subscribe( &engine );

      engine.run(scenario.get());

      std::cerr << "=== Feasible CP Solution ===" << std::endl;
      std::cerr << cpSolution.stringify() << std::endl;
      std::cerr << "Errors: " << cpSolution.errors() << std::endl;
      std::cerr << "Complete: " << cpSolution.complete() << std::endl;

      // Now try SCIP solver
      const auto& model = constraintProgramm.getModel();
      CP::SCIPSolver solver(model);

      // Get SCIP problem for inspection
      SCIP* scip = solver.getScip();
/*


      // Fix position variables to simplify the problem
      std::vector<std::pair<int, double>> positionFixes = {
        {0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 5}, {5, 8},
        {6, 6}, {7, 9}, {8, 7}, {9, 10}, {10, 12}, {11, 11},
        {12, 13}, {13, 14}
      };

      for (const auto& [idx, val] : positionFixes) {
        std::string varName = "position[" + std::to_string(idx) + "]";
        SCIP_VAR* var = SCIPfindVar(scip, varName.c_str());
        if (var) {
          SCIPchgVarLb(scip, var, val);
          SCIPchgVarUb(scip, var, val);
          std::cerr << "Fixed " << varName << " = " << val << std::endl;
        } else {
          std::cerr << "Variable " << varName << " not found" << std::endl;
        }
      }

      SCIPwriteOrigProblem(scip, "debug_problem.cip", "cip", FALSE);
      std::cerr << "Saved SCIP problem to debug_problem.cip" << std::endl;
*/
      // Enable presolving to trigger the bug
      SCIPsetIntParam(scip, "presolving/maxrounds", 2);
      SCIPsetIntParam(scip, "display/verblevel", 5);

      auto result = solver.solve(model);

      // Save transformed/presolved problem
//      SCIPwriteTransProblem(scip, "debug_presolved.cip", "cip", FALSE);
//      std::cerr << "Saved presolved problem to debug_presolved.cip" << std::endl;

      // Print SCIP status
      SCIP_STATUS status = SCIPgetStatus(scip);
      std::cerr << "SCIP status: " << status << std::endl;
      if (!result.has_value()) {
        std::cerr << "No SCIP solution: " << result.error() << std::endl;

        // Try to check the feasible solution against SCIP constraints
        std::cerr << "\n=== Checking CP solution against SCIP model ===" << std::endl;
        const auto& cpSol = cpSolution.getSolution();
        std::cerr << "CP Solution errors: " << cpSol.errors() << std::endl;
      }

      THEN( "An optimal solution is found" ) {
        REQUIRE( result.has_value() );
      }
    }
  }
};


//#include "cpmodel/test.h"
//#include "cpsolver/test.h"
//#include "debug.h"
//#include "examples/knapsack_problem/test.h"
//#include "cp/examples/knapsack_problem/test.h"
//#include "cp/examples/bin_packing_problem/test.h"
//#include "cp/examples/job_shop_scheduling_problem/test.h"
//#include "cp/test.h"
//#include "cp/multiinstanceactivity/test.h"
//#include "cp/loopactivity/test.h"
//#include "cp/process/test.h"
//#include "cp/exclusivegateway/test.h"
//#include "cp/eventbasedgateway/test.h"
//#include "cp/adhocsubprocess/test.h"
//#include "cp/message/test.h"
#endif // ALL_TESTS

#include <regex>
// Playground
void test() {
  // add code to test here
  std::string jsonString = "{\"distribution\": \"uniform_int_distribution\", \"min\": 0, \"max\": 10}";
  auto distribution = make_distribution(jsonString);

	RandomGenerator gen{std::random_device{}()};
	for(int i = 0; i < 10; ++i) {
		std::cout << distribution(gen) << '\n';
  }

std::cerr << "Load BPMN model" << std::endl;
  const std::string modelFile = "execution/adhocsubprocess/AdHocSubProcess.bpmn";
  BPMN::Model basicmodel(modelFile);
std::cerr << "BPMN model did not throw" << std::endl;
std::cerr << "Load BPMNOS model" << std::endl;
  BPMNOS::Model::Model model(modelFile);
std::cerr << "BPMNOS model did not throw" << std::endl;
}

/*
TEST_CASE("My Test Case") {
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
}
*/
