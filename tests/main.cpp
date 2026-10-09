#include "prelude.h"
#include <catch2/catch_test_case_info.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#define CATCH_CONFIG_NO_THROW
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

// The tests are the translation units listed in tests_SOURCES in CMakeLists.txt.

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

