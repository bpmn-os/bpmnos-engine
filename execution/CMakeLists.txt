# No project() here: this is a component of the engine, not a project of its own. Declaring one made
# PROJECT_NAME and PROJECT_SOURCE_DIR mean something different in each subdirectory, which is the ambiguity
# bpmn++ fell foul of, and it reset the language standard the top level had already set.

message("-- Library will be created in folder: ${BPMNOS_LIB_DIR}")

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BPMNOS_LIB_DIR})

# Get lists of all headers and sources of utilities
include("${CMAKE_CURRENT_SOURCE_DIR}/utility/CMakeLists.txt")

# Get lists of all headers and sources of BPMN engine
include("${CMAKE_CURRENT_SOURCE_DIR}/engine/CMakeLists.txt")

# Get lists of all headers and sources of observers
include("${CMAKE_CURRENT_SOURCE_DIR}/observer/CMakeLists.txt")

# Get lists of all headers and sources of execution model
include("${CMAKE_CURRENT_SOURCE_DIR}/model/CMakeLists.txt")

# Get lists of all headers and sources of controllers
include("${CMAKE_CURRENT_SOURCE_DIR}/controller/CMakeLists.txt")


# Generate the single header file
set(SINGLE_HEADER_PATH "${BPMNOS_INCLUDE_DIR}/bpmnos-execution.h")
message("-- Header location: ${SINGLE_HEADER_PATH}")

# Create output directory for the library
file(MAKE_DIRECTORY "${BPMNOS_INCLUDE_DIR}")

set(ALL_HEADERS
  ${execution_utility_HEADERS}
  ${engine_HEADERS}
  ${executionmodel_HEADERS}
  ${observer_HEADERS}
  ${controller_HEADERS}
)
string(JOIN " " HEADERS_JOINED ${ALL_HEADERS})
#message("-- Header files to be joined: ${HEADERS_JOINED}")

# Add a custom command to generate the header
add_custom_command(
  OUTPUT "${SINGLE_HEADER_PATH}"
  COMMAND ${CMAKE_COMMAND}
    -DHEADERS="${HEADERS_JOINED}"
    -DOUTPUT=${SINGLE_HEADER_PATH}
    -DPROLOGUE="bpmn++.h bpmnos-model.h"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/../single-header.cmake"
  DEPENDS ${ALL_HEADERS}
  COMMENT "Generating single header: ${SINGLE_HEADER_PATH}"
  VERBATIM
)

# Add a custom target that always ensures the header is generated
add_custom_target(execution_header ALL
  DEPENDS "${SINGLE_HEADER_PATH}"
)

add_library(bpmnos-execution STATIC ${execution_utility_SOURCES} ${engine_SOURCES} ${executionmodel_SOURCES} ${observer_SOURCES} ${controller_SOURCES})

bpmnos_compile_options(bpmnos-execution)

# Anything linking this library includes the amalgamated header, so the library waits for it to be
# generated. Without this the app could compile before the header existed, which went unnoticed only
# because the header used to persist in the checkout between builds.
add_dependencies(bpmnos-execution execution_header)

# A consumer names the same target whether the engine was built in its tree or resolved from a prefix, and
# inherits the amalgamated header's directory and bpmn++ from it rather than restating either.
# The exported name is set explicitly: install(EXPORT ... NAMESPACE bpmnos::) prefixes the target name,
# which would give bpmnos::bpmnos-execution and disagree with the alias used in a build tree.
set_target_properties(bpmnos-execution PROPERTIES EXPORT_NAME execution)
add_library(bpmnos::execution ALIAS bpmnos-execution)

target_include_directories(bpmnos-execution
  PUBLIC
    $<BUILD_INTERFACE:${BPMNOS_INCLUDE_DIR}>
    "$<BUILD_INTERFACE:${BPMNOS_THIRDPARTY_INCLUDE_DIRS}>"
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(bpmnos-execution PUBLIC bpmnpp::bpmn++)

target_compile_features(bpmnos-execution PUBLIC cxx_std_23)

include(GNUInstallDirs)

# Install the header files
install(FILES ${SINGLE_HEADER_PATH} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the library
install(TARGETS bpmnos-execution
  EXPORT  bpmnos-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

