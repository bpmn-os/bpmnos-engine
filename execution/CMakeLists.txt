cmake_minimum_required(VERSION 3.26.4)

project(bpmnos-execution)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

message("-- Library will be created in folder: ${BPMNOS_LIB_DIR}")

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BPMNOS_LIB_DIR})

# Get lists of all headers and sources of utilities
include("${PROJECT_SOURCE_DIR}/utility/CMakeLists.txt")

# Get lists of all headers and sources of BPMN engine
include("${PROJECT_SOURCE_DIR}/engine/CMakeLists.txt")

# Get lists of all headers and sources of observers
include("${PROJECT_SOURCE_DIR}/observer/CMakeLists.txt")

# Get lists of all headers and sources of execution model
include("${PROJECT_SOURCE_DIR}/model/CMakeLists.txt")

# Get lists of all headers and sources of controllers
include("${PROJECT_SOURCE_DIR}/controller/CMakeLists.txt")


# Generate the single header file
set(SINGLE_HEADER_PATH "${BPMNOS_INCLUDE_DIR}/${PROJECT_NAME}.h")
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
    -P "${PROJECT_SOURCE_DIR}/../single-header.cmake"
  DEPENDS ${ALL_HEADERS}
  COMMENT "Generating single header: ${SINGLE_HEADER_PATH}"
  VERBATIM
)

# Add a custom target that always ensures the header is generated
add_custom_target(execution_header ALL
  DEPENDS "${SINGLE_HEADER_PATH}"
)

add_library(${PROJECT_NAME} STATIC ${execution_utility_SOURCES} ${engine_SOURCES} ${executionmodel_SOURCES} ${observer_SOURCES} ${controller_SOURCES})

if (NDEBUG)
  target_compile_options(${PROJECT_NAME}
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
  )
else()
  target_compile_options(${PROJECT_NAME}
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
  )

  add_link_options(-fsanitize=address,undefined,leak)

  target_link_options(${PROJECT_NAME}
    PRIVATE
    -fsanitize=address,undefined,leak
    -pg # Profiling
  )
endif()

# Anything linking this library includes the amalgamated header, so the library waits for it to be
# generated. Without this the app could compile before the header existed, which went unnoticed only
# because the header used to persist in the checkout between builds.
add_dependencies(${PROJECT_NAME} execution_header)

# A consumer names the same target whether the engine was built in its tree or resolved from a prefix, and
# inherits the amalgamated header's directory and bpmn++ from it rather than restating either.
add_library(bpmnos::execution ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${BPMNOS_INCLUDE_DIR}>
    "$<BUILD_INTERFACE:${BPMNOS_THIRDPARTY_INCLUDE_DIRS}>"
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC bpmnpp::bpmn++)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

include(GNUInstallDirs)

# Install the header files
install(FILES ${SINGLE_HEADER_PATH} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install the library
install(TARGETS ${PROJECT_NAME}
  EXPORT  bpmnos-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

