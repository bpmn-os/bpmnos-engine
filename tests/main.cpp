#include <catch2/catch_test_macros.hpp>
#define CATCH_CONFIG_NO_THROW
#include <bpmn++.h>
#include "bpmnos-model.h"
#include "bpmnos-execution.h"

using namespace BPMNOS;

// Include all tests here
#include "ParseElements/test.h"
#include "StaticData/test.h"
#include "DynamicData/test.h"
#include "Expression/test.h"
#include "Execution/test.h"

// Playground
#include <iostream>
void test() {
  // add code to test here
  std::string jsonString = "{\"distribution\": \"uniform_int_distribution\", \"min\": 0, \"max\": 10}";
  auto distribution = make_distribution(jsonString);

	RandomGenerator gen{std::random_device{}()};
	for(int i = 0; i < 10; ++i) {
		std::cout << distribution(gen) << '\n';
  }

}
/*
TEST_CASE("My Test Case") {
    test();
}
*/
