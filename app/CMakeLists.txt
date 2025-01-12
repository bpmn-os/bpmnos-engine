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
  target_compile_options(bpmnos
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

  target_link_options(bpmnos
    PRIVATE
    -fsanitize=address,undefined,leak
    -pg # Profiling
  )  
endif()

