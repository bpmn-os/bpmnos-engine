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
#include "model/expression/test.h"
#include "model/lookup/test.h"
/* Data provider */
#include "data/static/test.h"
#include "data/dynamic/test.h"
/* Execution engine */
// Process
#include "execution/process/test.h"

// Activities
#include "execution/task/test.h"
#include "execution/subprocess/test.h"

// Gateways
#include "execution/parallelgateway/test.h"
#include "execution/exclusivegateway/test.h"

// Event-based gateways
#include "execution/eventbasedgateway/test.h"
// Events
#include "execution/timer/test.h"
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
#include "execution/multiinstanceactivity/test.h"
// Ad-hoc subprocesses
#include "execution/adhocsubprocess/test.h"

// Status
#include "execution/status/test.h"

// Data
#include "execution/data/test.h"

// Examples
#include "examples/travelling_salesperson_problem/test.h"
#include "examples/assignment_problem/test.h"
#include "examples/knapsack_problem/test.h"
#include "examples/bin_packing_problem/test.h"
#include "examples/job_shop_scheduling_problem/test.h"

#endif // ALL_TESTS

#ifndef ALL_TESTS
SCENARIO( "Bin packing problem", "[examples][bin_packing_problem]" ) {
  const std::string modelFile = "examples/bin_packing_problem/Guided_bin_packing_problem.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "Three bins and three items" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      ";;Items;3\n"
      ";;Bins;3\n"
      "BinProcess;Bin1;Capacity;40\n"
      "BinProcess;Bin2;Capacity;40\n"
      "BinProcess;Bin3;Capacity;40\n"
      "ItemProcess;Item1;Size;20\n"
      "ItemProcess;Item2;Size;15\n"
      "ItemProcess;Item3;Size;22\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with the guided controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      readyHandler.connect(&engine);
      completionHandler.connect(&engine);

      Execution::GuidedEvaluator evaluator;
      Execution::GreedyController controller(&evaluator);
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

//      Execution::Recorder recorder;
      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then the solution considers all items" ) {
        auto failureLog = recorder.find(nlohmann::json{{"state", "FAILED"}});
        REQUIRE( failureLog.size() == 0 );
      }
/*
      THEN( "Then the knapsack handles items with best value to weight ratio first" ) {
        auto handleLog = recorder.find(nlohmann::json{{"nodeId", "HandleItemActivity"},{"state", "COMPLETED"}});
        REQUIRE( handleLog[0]["status"]["item"] == "Item3" );
        REQUIRE( handleLog[1]["status"]["item"] == "Item1" );
        REQUIRE( handleLog[2]["status"]["item"] == "Item2" );
      }
      THEN( "Then the knapsack includes Item3 and Item2" ) {
        auto acceptanceLog = recorder.find(nlohmann::json{{"nodeId", "ItemAccepted"},{"state", "ENTERED"}});
        REQUIRE( acceptanceLog[0]["instanceId"] == "Item3" );
        REQUIRE( acceptanceLog[1]["instanceId"] == "Item2" );
      }
*/
    }
  }
}
#endif // ALL_TESTS

// When adding/modifying tests, don't forget to run: make clean; cmake ..; make all -j7; make tests


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


TEST_CASE("My Test Case") {
//    test();
}

