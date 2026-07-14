cmake_minimum_required(VERSION 3.26.4)

project(bpmnos-greedy)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

# Set the output directory for the binary
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../bin)

include_directories(
  ${PROJECT_SOURCE_DIR}/../lib
)

# Add the executable
add_executable(bpmnos-greedy main.cpp)

# Add libraries
target_link_libraries(bpmnos-greedy
  PRIVATE 
  bpmnos-execution
  bpmnos-model
  bpmn++
  pthread
  xerces-c
)

if (NDEBUG)
  target_compile_options(bpmnos-greedy
    PRIVATE
    -O3
    -Werror
    -pedantic-errors
    -Wpedantic
    -Wall
    ${BPMNOS_SHADOW_FLAG}
    -Wextra
    -Wconversion
    -Wsign-conversion
    ${BPMNOS_WARNING_FLAGS}
    ${BPMNOS_MAX_ERRORS_FLAG}
    -Wno-error=unused-parameter # Remove
    -Wno-error=unused-variable # Remove
    -Wno-misleading-indentation
)
else()
  target_compile_options(bpmnos-greedy
    PRIVATE
    -O0
    -Werror
    -pedantic-errors
    -Wpedantic
    -Wall
    ${BPMNOS_SHADOW_FLAG}
    -Wextra
    -Wconversion
    -Wsign-conversion
    ${BPMNOS_WARNING_FLAGS}
    -fsanitize=address,undefined,leak
    -fno-omit-frame-pointer
    -g
    -ggdb3
    -pg # Profiling
    ${BPMNOS_MAX_ERRORS_FLAG}
    -Wno-error=unused-parameter # Remove
    -Wno-error=unused-variable # Remove
    -Wno-misleading-indentation
  )

  add_link_options(-fsanitize=address,undefined,leak)

  target_link_options(bpmnos-greedy
    PRIVATE
    -fsanitize=address,undefined,leak
    -pg # Profiling
  )  
endif()

