include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Catch2
FetchContent_Declare(
        catch
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.4.0
)

FetchContent_GetProperties(catch)

if (NOT catch_POPULATED)
  FetchContent_Populate(catch)
  add_subdirectory(${catch_SOURCE_DIR} ${catch_BINARY_DIR})
endif ()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Add the -ggdb3 flag for debugging information
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb3")

# Add the -O0 flag to disable optimizations for better debugging with Valgrind
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

add_executable(run_tests EXCLUDE_FROM_ALL main.cpp)

include_directories(
  ${CMAKE_SOURCE_DIR}/lib
  /home/asvin/Code/bpmnos/cp/ # TODO
)

if (DEBUG)
  target_compile_options(run_tests
    PRIVATE
    -O0
    -Werror
    -pedantic-errors
    -Wpedantic
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
    -fsanitize=address
    -fno-omit-frame-pointer
    -g
    -ggdb3
    -fmax-errors=1 # Remove
    -Wno-error=unused-parameter # Remove
    -Wno-error=unused-variable # Remove
    -Wno-misleading-indentation
  )

  add_link_options(-fsanitize=address)

  target_link_options(run_tests
    PRIVATE
    -fsanitize=address
  )
else()
  target_compile_options(run_tests
    PRIVATE
    -O3
    -Werror
    -pedantic-errors
    -Wpedantic
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
    -fmax-errors=1 # Remove
    -Wno-error=unused-parameter # Remove
    -Wno-error=unused-variable # Remove
    -Wno-misleading-indentation
)
endif()


target_link_libraries(run_tests 
  PRIVATE 
  bpmnos-execution
  bpmnos-model
  bpmn++
  pthread
  nlohmann_json::nlohmann_json
  xerces-c
  Catch2::Catch2WithMain
)

# Copy all test data to build
file(GLOB test_files RELATIVE "${PROJECT_SOURCE_DIR}/tests" "${PROJECT_SOURCE_DIR}/tests/*")
foreach(file ${test_files})
  if(IS_DIRECTORY "${PROJECT_SOURCE_DIR}/tests/${file}")
#    if(NOT (file MATCHES ".*\\.cpp$" OR file MATCHES ".*\\.h$" OR file MATCHES ".*\\CMakeLists.txt"))
        # Copy the file to the destination directory, preserving the relative path
        file(COPY "${file}" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/")
  endif()
endforeach()
# Create a script that can be used from build directory
file(WRITE "${CMAKE_BINARY_DIR}/run_tests" "#!/bin/bash\nexport LSAN_OPTIONS=\"suppressions=${PROJECT_SOURCE_DIR}/leak.supp\"\n(cd ${CMAKE_BINARY_DIR}/tests/;./run_tests $@)")
execute_process(COMMAND chmod +x "${CMAKE_BINARY_DIR}/run_tests")
message("-- Run tests: ./run_test [tags]")

add_custom_target(
    tests
    COMMAND ${CMAKE_BINARY_DIR}/run_tests || exit 0
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/
    VERBATIM
)

add_dependencies(run_tests bpmnos-model bpmnos-execution)
add_dependencies(tests run_tests)

