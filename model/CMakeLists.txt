# No project() here: this is a component of the engine, not a project of its own. Declaring one made
# PROJECT_NAME and PROJECT_SOURCE_DIR mean something different in each subdirectory, which is the ambiguity
# bpmn++ fell foul of, and it reset the language standard the top level had already set.

message("-- Library will be created in folder: ${BPMNOS_LIB_DIR}")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BPMNOS_LIB_DIR})

# Get lists of all headers and sources of utilities
include("${CMAKE_CURRENT_SOURCE_DIR}/utility/CMakeLists.txt")

# Get lists of all headers and sources of BPMN parser
include("${CMAKE_CURRENT_SOURCE_DIR}/bpmnos/CMakeLists.txt")

# Get lists of all headers and sources of data reader
include("${CMAKE_CURRENT_SOURCE_DIR}/data/CMakeLists.txt")


# Generate the single header file
set(SINGLE_HEADER_PATH "${BPMNOS_INCLUDE_DIR}/bpmnos-model.h")
message("-- Header location: ${SINGLE_HEADER_PATH}")

# Create output directory for the library
file(MAKE_DIRECTORY "${BPMNOS_INCLUDE_DIR}")

set(ALL_HEADERS
  ${model_utility_HEADERS}
  ${bpmn_HEADERS}
  ${data_HEADERS}
)
string(JOIN " " HEADERS_JOINED ${ALL_HEADERS})
#message("-- Header files to be joined: ${HEADERS_JOINED}")

# Add a custom command to generate the header
add_custom_command(
  OUTPUT "${SINGLE_HEADER_PATH}"
  COMMAND ${CMAKE_COMMAND}
    -DHEADERS="${HEADERS_JOINED}"
    -DOUTPUT=${SINGLE_HEADER_PATH}
    -DPROLOGUE="bpmn++.h"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/../single-header.cmake"
  DEPENDS ${ALL_HEADERS}
  COMMENT "Generating single header: ${SINGLE_HEADER_PATH}"
  VERBATIM
)

# Add a custom target that always ensures the header is generated
add_custom_target(model_header ALL
  DEPENDS "${SINGLE_HEADER_PATH}"
)

add_library(bpmnos-model STATIC ${model_utility_SOURCES} ${bpmn_SOURCES} ${data_SOURCES})

# Delete xml directory when running `make clean`
set_property(
  TARGET bpmnos-model
  APPEND
  PROPERTY ADDITIONAL_CLEAN_FILES "${XML_DIR}"
)

bpmnos_compile_options(bpmnos-model)

# Anything linking this library includes the amalgamated header, so the library waits for it to be
# generated. Without this the app could compile before the header existed, which went unnoticed only
# because the header used to persist in the checkout between builds.
add_dependencies(bpmnos-model model_header)

# A consumer names the same target whether the engine was built in its tree or resolved from a prefix, and
# inherits the amalgamated header's directory and bpmn++ from it rather than restating either.
# The exported name is set explicitly: install(EXPORT ... NAMESPACE bpmnos::) prefixes the target name,
# which would give bpmnos::bpmnos-model and disagree with the alias used in a build tree.
set_target_properties(bpmnos-model PROPERTIES EXPORT_NAME model)
add_library(bpmnos::model ALIAS bpmnos-model)

target_include_directories(bpmnos-model
  PUBLIC
    $<BUILD_INTERFACE:${BPMNOS_INCLUDE_DIR}>
    "$<BUILD_INTERFACE:${BPMNOS_THIRDPARTY_INCLUDE_DIRS}>"
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(bpmnos-model PUBLIC bpmnpp::bpmn++)

target_compile_features(bpmnos-model PUBLIC cxx_std_23)

include(GNUInstallDirs)

# Install the header files
install(FILES ${SINGLE_HEADER_PATH} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the library
install(TARGETS bpmnos-model
  EXPORT  bpmnos-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

