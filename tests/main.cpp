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
#endif // ALL_TESTS

#ifndef ALL_TESTS
SCENARIO( "Recursive compensations", "[execution][compensation]" ) {
  const std::string modelFile = "execution/compensationeventsubprocess/Recursive_compensations.bpmn";
  REQUIRE_NOTHROW( Model::Model(modelFile) );
  GIVEN( "A single instance with no input values" ) {

    std::string csv =
      "PROCESS_ID, INSTANCE_ID, ATTRIBUTE_ID, VALUE\n"
      "Process_1, Instance_1,,\n"
    ;

    Model::StaticDataProvider dataProvider(modelFile,csv);
    auto scenario = dataProvider.createScenario();

    WHEN( "The engine is started with a recorder" ) {
      Execution::Engine engine;
      Execution::ReadyHandler readyHandler;
      Execution::InstantEntryHandler entryHandler;
      Execution::DeterministicTaskCompletionHandler completionHandler;
      Execution::InstantExitHandler exitHandler;
      Execution::TimeWarp timeHandler;
      engine.addEventHandler(&readyHandler);
      engine.addEventHandler(&entryHandler);
      engine.addEventHandler(&completionHandler);
      engine.addEventHandler(&exitHandler);
      engine.addEventHandler(&timeHandler);
//      Execution::Recorder recorder;
      Execution::Recorder recorder(std::cerr);
      engine.addListener(&recorder);
      engine.run(scenario.get());
      THEN( "Then the nodes change their states in the correct order" ) {
        auto entryLog = recorder.find(nlohmann::json{{"state", "ENTERED"}});
        REQUIRE( entryLog[0]["nodeId"] == nullptr );
        REQUIRE( entryLog[1]["nodeId"] == "StartEvent_1" );
        REQUIRE( entryLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( entryLog[3]["nodeId"] == "StartEvent_2" );
        REQUIRE( entryLog[4]["nodeId"] == "Activity_1" );
        REQUIRE( entryLog[5]["nodeId"] == "Activity_2" );
        REQUIRE( entryLog[6]["nodeId"] == "EndEvent_2" );
        REQUIRE( entryLog[7]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( entryLog[8]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( entryLog[9]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( entryLog[10]["nodeId"] == "CompensationActivity_1" );

        auto completionLog = recorder.find(nlohmann::json{{"state", "COMPLETED"}});
        REQUIRE( completionLog[0]["nodeId"] == "Activity_1" );
        REQUIRE( completionLog[1]["nodeId"] == "Activity_2" );
        REQUIRE( completionLog[2]["nodeId"] == "SubProcess_1" );
        REQUIRE( completionLog[3]["nodeId"] == "CompensateStartEvent_1" );
        REQUIRE( completionLog[4]["nodeId"] == "CompensateBoundaryEvent_2" );
        REQUIRE( completionLog[5]["nodeId"] == "CompensationActivity_2" );
        REQUIRE( completionLog[6]["nodeId"] == "CompensateBoundaryEvent_1" );
        REQUIRE( completionLog[7]["nodeId"] == "CompensationActivity_1" );
        REQUIRE( completionLog[8]["nodeId"] == "CompensateThrowEvent_3" );
        REQUIRE( completionLog[9]["nodeId"] == "CompensateThrowEvent_1" );
        REQUIRE( completionLog[10]["nodeId"] == nullptr );
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
    // exprtk

  exprtk::symbol_table<double> symbolTable;
  exprtk::expression<double> expression;
  exprtk::parser<double> parser;

  expression.register_symbol_table(symbolTable);

  parser.enable_unknown_symbol_resolver();

  std::string user_expression = "2 * x[i] + y";
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

}


TEST_CASE("My Test Case") {
    //test();
}


