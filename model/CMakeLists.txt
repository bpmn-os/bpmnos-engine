cmake_minimum_required(VERSION 3.26.4)

project(bpmnos-model)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

message("-- Library will be created in folder: ${BPMNOS_LIB_DIR}")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${BPMNOS_LIB_DIR})

# Get lists of all headers and sources of utilities
include("${PROJECT_SOURCE_DIR}/utility/CMakeLists.txt")

# Get lists of all headers and sources of BPMN parser
include("${PROJECT_SOURCE_DIR}/bpmnos/CMakeLists.txt")

# Get lists of all headers and sources of data reader
include("${PROJECT_SOURCE_DIR}/data/CMakeLists.txt")


# Generate the single header file
set(SINGLE_HEADER_PATH "${BPMNOS_INCLUDE_DIR}/${PROJECT_NAME}.h")
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
    -P "${PROJECT_SOURCE_DIR}/../single-header.cmake"
  DEPENDS ${ALL_HEADERS}
  COMMENT "Generating single header: ${SINGLE_HEADER_PATH}"
  VERBATIM
)

# Add a custom target that always ensures the header is generated
add_custom_target(model_header ALL
  DEPENDS "${SINGLE_HEADER_PATH}"
)

add_library(${PROJECT_NAME} STATIC ${model_utility_SOURCES} ${bpmn_SOURCES} ${data_SOURCES})

# Delete xml directory when running `make clean`
set_property(
  TARGET ${PROJECT_NAME}
  APPEND
  PROPERTY ADDITIONAL_CLEAN_FILES "${XML_DIR}"
)

if (NDEBUG)
  target_compile_options(${PROJECT_NAME}
    PRIVATE
    -O3
    -Werror
    ${BPMNOS_SHADOW_FLAG}
    -pedantic-errors
    -Wpedantic
    -Wall
    -Wextra
    -Wconversion
    -Wsign-conversion
    ${BPMNOS_WARNING_FLAGS}
    ${BPMNOS_MAX_ERRORS_FLAG}
    -Wno-error=unused-parameter # Remove
    -Wno-error=unused-variable # Remove
# Add below line to suppress warnings for CSV Parser (not necessary for -O2)
#    -Wno-array-bounds 
#    -Wno-stringop-overread
  )
else()
  target_compile_options(${PROJECT_NAME}
    PRIVATE
    -O0
    -Werror
    ${BPMNOS_SHADOW_FLAG}
    -pedantic-errors
    -Wpedantic
    -Wall
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
add_dependencies(${PROJECT_NAME} model_header)

# A consumer names the same target whether the engine was built in its tree or resolved from a prefix, and
# inherits the amalgamated header's directory and bpmn++ from it rather than restating either.
add_library(bpmnos::model ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${BPMNOS_INCLUDE_DIR}>
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

