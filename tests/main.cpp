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

// Examples
#include "examples/assignment_problem/test.h"
#include "examples/knapsack_problem/test.h"
#endif // ALL_TESTS

#ifndef ALL_TESTS
#include "execution/process/test.h"
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
    // exprtk

  exprtk::symbol_table<double> symbolTable;
  exprtk::expression<double> expression;
  exprtk::parser<double> parser;

  expression.register_symbol_table(symbolTable);

  parser.enable_unknown_symbol_resolver();

  std::string user_expression = "not(2 * x[i] > y)";
/*
  if (auto result = parser.compile(user_expression, expression); !result) {
    throw std::runtime_error("GenericExpression: compilation of expression failed with: " + parser.error());
  }

  // get variable names used in expression
  std::list<std::string> variables;
  symbolTable.get_variable_list(variables);

    // Print out the variables
    std::cout << "Variables used in the expression:\n";
    for (const auto& variable : variables) {
        std::cout << variable << "\n";
    }

  std::set<std::string> indices;
  std::set<std::string> vectors;

// Regular expression to match symbols followed by indices
    std::regex vector_regex(R"((\w+)\[(\w+)\])");

// Iterate over matches in the expression string
    auto words_begin = std::sregex_iterator(user_expression.begin(), user_expression.end(), vector_regex);
    auto words_end = std::sregex_iterator();
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        // Extract the symbol and index from the match
        std::string symbol = match[1];
        std::string index = match[2];
        vectors.insert(symbol); // Add the symbol to the set of vectors
        indices.insert(index); // Add the index to the set of variables
    }

    // Print out the vectors
    std::cout << "Vectors used in the expression:\n";
    for (const auto& vector : vectors) {
        std::cout << vector << "\n";
    }

    // Print out the indices
    std::cout << "Indices used in the expression:\n";
    for (const auto& index : indices) {
        std::cout << index << "\n";
    }
*/
    // Populate the data
    std::vector<double> x = {1.0, 2.0, 3.0};
    double y = .5;
    double i = 1;

    symbolTable.add_vector("x", x.data(), x.size());
    symbolTable.add_variable("y", y);
    symbolTable.add_variable("i", i);

    expression.register_symbol_table(symbolTable);
parser.compile(user_expression, expression);
    std::cout << "Result for i = " << i << ": " << expression.value() << std::endl;

    BPMNOS::number n = 0;
    n += BPMNOS_NUMBER_PRECISION;
    std::cout << "Numeric precision: " << to_rep(n) << "/" << (double)n << std::endl;
//    BPMNOS::number m = cnl::from_rep< int, cnl::power<-16> >(r);
    
    std::cerr << "Element of: " << "∈" << ", not element of: " << "∉" << std::endl;
}


TEST_CASE("My Test Case") {
//    test();
}


