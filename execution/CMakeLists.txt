cmake_minimum_required(VERSION 3.26.4)

project(bpmnos-execution)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

# Set folder for the library to be created
set(LIB_DIR "lib" CACHE STRING "Library directory")
message("-- Library will be created in folder: ${PROJECT_SOURCE_DIR}/../${LIB_DIR}")

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/../${LIB_DIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

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

find_program(CCACHE_FOUND "ccache")
set(CCACHE_SUPPORT ON CACHE BOOL "Enable ccache support")
if (CCACHE_FOUND AND CCACHE_SUPPORT)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "ccache")
endif()

# Generate the single header file
set(SINGLE_HEADER_PATH "${PROJECT_SOURCE_DIR}/../${LIB_DIR}/${PROJECT_NAME}.h")
message("-- Header location: ${SINGLE_HEADER_PATH}")

# Create output directory for the library
file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/../${LIB_DIR}")

set(ALL_HEADERS
  ${execution_utility_HEADERS}
  ${engine_HEADERS}
  ${observer_HEADERS}
  ${executionmodel_HEADERS}
  ${controller_HEADERS}
)
string(JOIN " " HEADERS_JOINED ${ALL_HEADERS})
#message("-- Header files to be joined: ${HEADERS_JOINED}")

# Add a custom command to generate the header
add_custom_command(
  OUTPUT "${SINGLE_HEADER_PATH}"
  COMMAND ${CMAKE_COMMAND}
    -DHEADERS="${HEADERS_JOINED}"
    -DFILENAME=${PROJECT_NAME}.h
    -P "${PROJECT_SOURCE_DIR}/../single-header.cmake"
  DEPENDS ${ALL_HEADERS}
  COMMENT "Generating single header: ${SINGLE_HEADER_PATH}"
  VERBATIM
)

# Add a custom target that always ensures the header is generated
add_custom_target(execution_header ALL
  DEPENDS "${SINGLE_HEADER_PATH}"
)

add_library(${PROJECT_NAME} STATIC ${execution_utility_SOURCES} ${engine_SOURCES} ${observer_SOURCES} ${executionmodel_SOURCES} ${controller_SOURCES})

if (NDEBUG)
  target_compile_options(${PROJECT_NAME}
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
  )
else()
  target_compile_options(${PROJECT_NAME}
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
  )

  add_link_options(-fsanitize=address,undefined,leak)

  target_link_options(${PROJECT_NAME}
    PRIVATE
    -fsanitize=address,undefined,leak
    -pg # Profiling
  )
endif()

# Install the header files
install(FILES ${SINGLE_HEADER_PATH} DESTINATION include)

# Install the library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)

