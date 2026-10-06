#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#define CATCH_CONFIG_NO_THROW
// Provided transitively by bpmnos-execution.h, which is self-contained; named here to
// state what this file uses, and commented out so that it stays a check on that.
//#include <bpmn++.h>
//#include <bpmnos-model.h>
#include <bpmnos-execution.h>
#include <iostream>

class ProgressListener : public Catch::EventListenerBase {
public:
    using Catch::EventListenerBase::EventListenerBase;

    // Reported from the macros the compiler actually saw, not from what CMake believes it passed, so a run
    // that silently lost its assertions says so rather than passing quietly.
    void testRunStarting(Catch::TestRunInfo const&) override {
#ifdef NDEBUG
      std::cerr << "assertions DISABLED (NDEBUG defined)";
#else
      std::cerr << "assertions enabled";
#endif
#if defined(__SANITIZE_ADDRESS__) || (defined(__has_feature) && __has_feature(address_sanitizer))
      std::cerr << ", sanitizers on" << std::endl;
#else
      std::cerr << ", sanitizers off" << std::endl;
#endif
    }

    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        std::cerr << testInfo.name << " " << std::flush;
    }

    void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
        if (testCaseStats.totals.assertions.failed == 0) {
            std::cerr << "\033[32m[pass]\033[0m" << std::endl;
        }
/*
        else {
            std::cerr << "\033[31m[fail]\033[0m" << std::endl;
        }
*/
    }
};

CATCH_REGISTER_LISTENER(ProgressListener)

using namespace BPMNOS;

// Include all tests here
#define ALL_TESTS
#ifdef ALL_TESTS

/* Model */
#include "model/encoder/test.h"
#include "model/parser/test.h"
/* Data provider */
#include "data/static/test.h"
#include "data/dynamic/test.h"
#include "data/stochastic/test.h"
#include "data/reader/test.h"
#include "data/staticprovider/test.h"
#include "data/expectedvalueprovider/test.h"
#include "data/dynamicprovider/test.h"
#include "data/stochasticprovider/test.h"
#include "data/forking/test.h"
#include "data/parity/test.h"
#include "data/provider/test.h"
/* Execution engine */
// Process
#include "execution/process/test.h"

// Activities
#include "execution/task/test.h"
#include "execution/subprocess/test.h"
#include "execution/decisiontask/test.h"

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
#include "execution/triggeredprocess/test.h"
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

// SystemState
#include "systemstate/test.h"

// Candidate sources (notice(SystemState) rebuild path)
#include "candidates/test.h"

// Examples
#include "examples/earliest_arrival_problem/test.h"
#include "examples/travelling_salesperson_problem/test.h"
#include "examples/truck_driver_scheduling_problem/test.h"
#include "examples/assignment_problem/test.h"
#include "examples/knapsack_problem/test.h"
#include "examples/bin_packing_problem/test.h" 
#include "examples/job_shop_scheduling_problem/test.h"
#include "examples/vehicle_routing_problem/test.h"
#include "examples/pickup_delivery_problem/test.h"

// TODO: Shaped examples

#endif // ALL_TESTS

#ifndef ALL_TESTS

//#include "cpmodel/test.h"
//#include "cpsolver/test.h"
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

