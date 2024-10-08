cmake_minimum_required(VERSION 3.26.4)

project(bpmnos-app)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

# Set the output directory for the binary
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(
  ${CMAKE_SOURCE_DIR}/lib
)

# Add the executable
add_executable(bpmnos main.cpp)

# Add libraries
target_link_libraries(bpmnos
  PRIVATE 
  bpmnos-execution
  bpmnos-model
  bpmn++
  pthread
  nlohmann_json::nlohmann_json
  xerces-c
)

if (NDEBUG)
  target_compile_options(bpmnos
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
)
else()
  target_compile_options(bpmnos
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
    -pg # Profiling
    -fmax-errors=1 # Remove
    -Wno-error=unused-parameter # Remove
    -Wno-error=unused-variable # Remove
  )

  add_link_options(-fsanitize=address)

  target_link_options(bpmnos
    PRIVATE
    -fsanitize=address
    -pg # Profiling
  )  
endif()

