#include <catch2/catch_test_macros.hpp>
#define CATCH_CONFIG_NO_THROW
#include <bpmn++.h>
#include "bpmnos-model.h"
#include "bpmnos-execution.h"

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
// Events
#include "execution/errorevent/test.h"
/*
#include "execution/escalationevent/test.h"
// Event subprocesses
#include "execution/eventsubprocess/test.h"
*/

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

#include <list>


#include <functional>

class Command {
public:
    template <typename Function, typename... Args>
    Command(void* a, void* b, Function&& f, Args&&... args)
      : function(std::bind(std::forward<Function>(f), std::forward<Args>(args)...)) {}

    void execute() {
        function();
    }

    // Make the class movable
    Command(Command&& other) noexcept
        : function(std::move(other.function)) {}

    // Make the class non-copyable
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;

private:
    std::function<void()> function;
};

void exampleFunction(int a, double b) {
    std::cout << "Example Function: " << a << ", " << b << std::endl;
}

void hello() {
    std::cout << "Hello world" << std::endl;
}

TEST_CASE("My Test Case") {
    test();
    std::list<Command> commands;
    commands.emplace_back(nullptr,nullptr,exampleFunction, 42, 3.14);
    commands.emplace_back(nullptr,nullptr,hello);

    for (auto& command : commands) {
        command.execute();
    }

}


