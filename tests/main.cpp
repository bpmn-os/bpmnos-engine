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
#include "examples/assignment_problem/test.h"
#include "examples/knapsack_problem/test.h"
#include "examples/travelling_salesperson_problem/test.h"

#endif // ALL_TESTS

#ifndef ALL_TESTS
SCENARIO( "Travelling salesperson problem", "[examples][travelling_salesperson_problem]" ) {
  const std::string modelFile = "examples/travelling_salesperson_problem/Travelling_salesperson_problem.bpmn";
  BPMNOS::Model::LookupTable::folders = { std::string(std::filesystem::current_path()) + "/examples/travelling_salesperson_problem" };
  REQUIRE_NOTHROW( Model::Model(modelFile) );

  GIVEN( "A TSP with four location" ) {

    std::string csv =
      "PROCESS_ID; INSTANCE_ID; ATTRIBUTE_ID; VALUE\n"
      "TravellingSalesperson_Process;Instance1;Origin;Hamburg\n"
      "TravellingSalesperson_Process;Instance1;Locations;[\"Munich\",\"Berlin\",\"Cologne\"]\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();
    WHEN( "The engine is started with the greedy controller" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::DeterministicTaskCompletion completionHandler;
      readyHandler.connect(&engine);
      completionHandler.connect(&engine);

      Execution::GreedyController controller;
      controller.connect(&engine);
      
      Execution::MyopicMessageTaskTerminator messageTaskTerminator;
      Execution::TimeWarp timeHandler;
      messageTaskTerminator.connect(&engine);
      timeHandler.connect(&engine);

//      Execution::Recorder recorder;
      Execution::Recorder recorder(std::cerr);
      recorder.subscribe(&engine);
      engine.run(scenario.get());
      THEN( "Then locations are visited in the nearest-neighbour order" ) {
        auto visitLog = recorder.find(nlohmann::json{{"nodeId", "VisitLocation"},{"state", "ENTERED"}});
        REQUIRE( visitLog[0]["status"]["location"] == "Berlin" );
        REQUIRE( visitLog[1]["status"]["location"] == "Cologne" );
        REQUIRE( visitLog[2]["status"]["location"] == "Munich" );
      }
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


