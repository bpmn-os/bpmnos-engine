include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Catch2
FetchContent_Declare(
        catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.12.0
)

FetchContent_GetProperties(catch)
FetchContent_MakeAvailable(catch)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

# Add the -ggdb3 flag for debugging information
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")

# Add the -O0 flag to disable optimizations for better debugging with Valgrind
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

# Each test file is a translation unit of its own, so that the suite compiles in parallel and a changed test
# recompiles alone. A new test file must be added here to run.
list(APPEND tests_SOURCES
  main.cpp
  model/encoder/test.cpp
  model/parser/test.cpp
  model/schema/test.cpp
  data/reader/test.cpp
  data/staticprovider/test.cpp
  data/expectedvalueprovider/test.cpp
  data/dynamicprovider/test.cpp
  data/stochasticprovider/test.cpp
  data/forking/test.cpp
  data/provider/test.cpp
  data/objects/test.cpp
  execution/process/test.cpp
  execution/task/test.cpp
  execution/subprocess/test.cpp
  execution/decisiontask/test.cpp
  execution/expression/test.cpp
  execution/parallelgateway/test.cpp
  execution/exclusivegateway/test.cpp
  execution/eventbasedgateway/test.cpp
  execution/timer/test.cpp
  execution/signal/test.cpp
  execution/triggeredprocess/test.cpp
  execution/condition/test.cpp
  execution/errorevent/test.cpp
  execution/escalationevent/test.cpp
  execution/message/test.cpp
  execution/boundaryevent/test.cpp
  execution/eventsubprocess/test.cpp
  execution/compensationactivity/test.cpp
  execution/compensationeventsubprocess/test.cpp
  execution/loopactivity/test.cpp
  execution/multiinstanceactivity/test.cpp
  execution/adhocsubprocess/test.cpp
  execution/status/test.cpp
  execution/data/test.cpp
  execution/collection/test.cpp
  systemstate/process/test.cpp
  systemstate/timer/test.cpp
  systemstate/signal/test.cpp
  systemstate/condition/test.cpp
  systemstate/task/test.cpp
  systemstate/message/test.cpp
  systemstate/eventbasedgateway/test.cpp
  systemstate/parallelgateway/test.cpp
  systemstate/multiinstanceactivity/test.cpp
  systemstate/compensationactivity/test.cpp
  systemstate/adhocsubprocess/test.cpp
  systemstate/eventsubprocess/test.cpp
  systemstate/compensationeventsubprocess/test.cpp
  candidates/rebuild/test.cpp
  candidates/resume/test.cpp
  examples/earliest_arrival_problem/test.cpp
  examples/travelling_salesperson_problem/test.cpp
  examples/truck_driver_scheduling_problem/test.cpp
  examples/assignment_problem/test.cpp
  examples/knapsack_problem/test.cpp
  examples/bin_packing_problem/test.cpp
  examples/job_shop_scheduling_problem/test.cpp
  examples/vehicle_routing_problem/test.cpp
  examples/pickup_delivery_problem/test.cpp
)

add_executable(run_tests EXCLUDE_FROM_ALL ${tests_SOURCES})

message("-- Include: ${BPMNOS_INCLUDE_DIR}")

include_directories(
  ${BPMNOS_INCLUDE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# The prelude every test file includes, parsed once rather than once per translation unit.
target_precompile_headers(run_tests PRIVATE prelude.h)

bpmnos_compile_options(run_tests)

target_link_libraries(run_tests
  PRIVATE
  bpmnos-execution
  bpmnos-model
  bpmnpp::bpmn++
  pthread
  Catch2::Catch2WithMain
)

if (CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  # Create a script that can be used from build directory
  file(WRITE "${CMAKE_BINARY_DIR}/run_tests" "#!/bin/bash\n(cd ${PROJECT_SOURCE_DIR};${CMAKE_BINARY_DIR}/tests/run_tests $@)")
  execute_process(COMMAND chmod +x "${CMAKE_BINARY_DIR}/run_tests")
  message("-- Run tests: ./run_tests [tags]")
endif()

add_custom_target(
    tests
    COMMAND ${CMAKE_BINARY_DIR}/run_tests || exit 0
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/
    VERBATIM
)

add_dependencies(run_tests bpmnos-model bpmnos-execution)
add_dependencies(tests run_tests)

