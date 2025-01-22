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
// TODO: change order of tests to avoid interdependencies

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
#include "examples/guided_bin_packing_problem/test.h" //TODO
#include "examples/travelling_salesperson_problem/test.h"
#include "examples/assignment_problem/test.h"
#include "examples/job_shop_scheduling_problem/test.h"
#include "examples/knapsack_problem/test.h"
#include "examples/guided_knapsack_problem/test.h"
#include "examples/vehicle_routing_problem/test.h"
#include "examples/guided_vehicle_routing_problem/test.h"
#include "examples/pickup_delivery_problem/test.h"
#include "examples/guided_pickup_delivery_problem/test.h"
#include "examples/truck_driver_scheduling_problem/test.h"

// CPController
//#include "cpcontroller/test.h"

#endif // ALL_TESTS

#ifndef ALL_TESTS
#include "examples/guided_bin_packing_problem/test.h" //TODO
//#include "examples/bin_packing_problem/test.h"
//#include "examples/pickup_delivery_problem/test.h" // TODO
//#include "cpcontroller/test.h"
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


TEST_CASE("My Test Case") {
//    test();
}

