#include <catch2/catch_test_macros.hpp>
#define CATCH_CONFIG_NO_THROW
#include <bpmn++.h>
#include "bpmnos-model.h"
#include "bpmnos-execution.h"
#include <iostream>

using namespace BPMNOS;

// Include all tests here

/* Model */
#include "model/parser/test.h"
#include "model/expression/test.h"
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
//#include "execution/boundaryevent/test.h"
// Event subprocesses

#include "execution/eventsubprocess/test.h"
// Compensations
#include "execution/compensationactivity/test.h"
#include "execution/compensationeventsubprocess/test.h"
// Multi-instance activities
#include "execution/multiinstanceactivity/test.h"

// Status
#include "execution/status/test.h"

// When adding/modifying tests, don't forget to run: make clean; cmake ..; make all -j7; make tests



// Playground
void test() {
  // add code to test here
  std::string jsonString = "{\"distribution\": \"uniform_int_distribution\", \"min\": 0, \"max\": 10}";
  auto distribution = make_distribution(jsonString);

	RandomGenerator gen{std::random_device{}()};
	for(int i = 0; i < 10; ++i) {
		std::cout << distribution(gen) << '\n';
  }
}


TEST_CASE("My Test Case") {
//    test();
}


