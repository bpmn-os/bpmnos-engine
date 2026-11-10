# No project() here: this is a component of the engine, not a project of its own. Declaring one made
# PROJECT_NAME and PROJECT_SOURCE_DIR mean something different in each subdirectory, which is the ambiguity
# bpmn++ fell foul of, and it reset the language standard the top level had already set.

# Set the output directory for the binary
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${BPMNOS_BINARY_DIR}/bin)

include_directories(
  ${BPMNOS_INCLUDE_DIR}
)

# Add the executable
add_executable(bpmnos-greedy main.cpp)

# Add libraries
target_link_libraries(bpmnos-greedy
  PRIVATE 
  bpmnos-execution
  bpmnos-model
  bpmnpp::bpmn++
  pthread
)

bpmnos_compile_options(bpmnos-greedy)

# The playback of the decisions of a log, which reproduces a run recorded by the engine or the run a log of
# decisions states, for instance the solution of a formulation of the model as a mixed-integer program.
add_executable(bpmnos-playback playback.cpp)

target_link_libraries(bpmnos-playback
  PRIVATE
  bpmnos-execution
  bpmnos-model
  bpmnpp::bpmn++
  pthread
)

bpmnos_compile_options(bpmnos-playback)

# The greedy solver is installed with the engine, so that a consumer resolving an installed engine has the
# baseline binary rather than having to build it.
include(GNUInstallDirs)
install(TARGETS bpmnos-greedy bpmnos-playback RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
