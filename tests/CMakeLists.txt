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

add_executable(run_tests EXCLUDE_FROM_ALL main.cpp)

message("-- Include: ${PROJECT_SOURCE_DIR}/lib")

include_directories(
  ${PROJECT_SOURCE_DIR}/lib
)

if (NDEBUG)
  target_compile_options(run_tests
    PRIVATE
    -O3
    -Werror
    -pedantic-errors
    -Wpedantic
    -Wall
    -Wshadow=local
    -Wextra
    -Wconversion
    -Wsign-conversion
    -fmax-errors=1 # Remove
    -Wno-error=unused-parameter # Remove
    -Wno-error=unused-variable # Remove
    -Wno-misleading-indentation
)
else()
  target_compile_options(run_tests
    PRIVATE
    -O0
    -Werror
    -pedantic-errors
    -Wpedantic
    -Wall
    -Wshadow=local
    -Wextra
    -Wconversion
    -Wsign-conversion
    -fsanitize=address,undefined,leak
    -fno-omit-frame-pointer
    -g
    -ggdb3
    -pg # Profiling
    -fmax-errors=1 # Remove
    -Wno-error=unused-parameter # Remove
    -Wno-error=unused-variable # Remove
    -Wno-misleading-indentation
  )

  add_link_options(-fsanitize=address,undefined,leak)

  target_link_options(run_tests
    PRIVATE
    -fsanitize=address,undefined,leak
    -pg # Profiling
  )
endif()

target_link_libraries(run_tests
  PRIVATE
  bpmnos-execution
  bpmnos-model
  bpmn++
  pthread
  xerces-c
  Catch2::Catch2WithMain
)

# Link CP-SCIP solver if available from engine build
find_library(CP_SCIP_LIB NAMES cp-scip PATHS ${CMAKE_BINARY_DIR}/_deps/cp-build/scip NO_DEFAULT_PATH)
if(CP_SCIP_LIB)
  message("-- Found CP-SCIP library: ${CP_SCIP_LIB}")

  # Find SCIP library from engine build
  find_library(SCIP_LIB NAMES scip PATHS ${CMAKE_BINARY_DIR}/_deps/scip-build/lib NO_DEFAULT_PATH)
  if(SCIP_LIB)
    message("-- Found SCIP library: ${SCIP_LIB}")
  else()
    message(WARNING "-- SCIP library not found")
  endif()

  # Find SCIP includes from engine build
  set(SCIP_SRC_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/scip-src/scip/src")
  set(SCIP_BUILD_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/scip-build/scip")
  set(SOPLEX_SRC_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/scip-src/soplex/src")
  set(SOPLEX_BUILD_INCLUDE_DIR "${CMAKE_BINARY_DIR}/_deps/scip-build/soplex")
  if(EXISTS ${SCIP_SRC_INCLUDE_DIR} AND EXISTS ${SCIP_BUILD_INCLUDE_DIR})
    message("-- Found SCIP source includes: ${SCIP_SRC_INCLUDE_DIR}")
    message("-- Found SCIP build includes: ${SCIP_BUILD_INCLUDE_DIR}")
    target_include_directories(run_tests PRIVATE
      ${SCIP_SRC_INCLUDE_DIR}
      ${SCIP_BUILD_INCLUDE_DIR}
      ${SOPLEX_SRC_INCLUDE_DIR}
      ${SOPLEX_BUILD_INCLUDE_DIR})
  else()
    message(WARNING "-- SCIP includes not found")
  endif()

  target_link_libraries(run_tests PRIVATE ${CP_SCIP_LIB} ${SCIP_LIB})
  target_compile_definitions(run_tests PRIVATE USE_SCIP_SOLVER)
else()
  message("-- CP-SCIP library not found, cpsolver tests will be disabled")
endif()

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

