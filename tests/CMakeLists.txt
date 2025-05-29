include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Catch2
FetchContent_Declare(
        catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.8.0
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

