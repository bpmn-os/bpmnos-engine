cmake_minimum_required(VERSION 3.26.4)

project(bpmnos-execution)

# Set folder for the library to be created
set(LIB_DIR "lib" CACHE STRING "Library directory")
message("-- Library will be created in folder: ${CMAKE_SOURCE_DIR}/${LIB_DIR}")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${LIB_DIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

# Get lists of all headers and sources of utilities
include("${PROJECT_SOURCE_DIR}/utility/CMakeLists.txt")

# Get lists of all headers and sources of BPMN engine
include("${PROJECT_SOURCE_DIR}/engine/CMakeLists.txt")

# Get lists of all headers and sources of observers
include("${PROJECT_SOURCE_DIR}/observer/CMakeLists.txt")

# Get lists of all headers and sources of controllers
include("${PROJECT_SOURCE_DIR}/controller/CMakeLists.txt")

find_program(CCACHE_FOUND "ccache")
set(CCACHE_SUPPORT ON CACHE BOOL "Enable ccache support")
if (CCACHE_FOUND AND CCACHE_SUPPORT)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "ccache")
endif()

# Generate the single header file
set(SINGLE_HEADER_PATH "${CMAKE_SOURCE_DIR}/${LIB_DIR}/${PROJECT_NAME}.h")
message("-- Header location: ${SINGLE_HEADER_PATH}")

message("-- Create single header file")

# Create output directory for the library
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/${LIB_DIR}")

# Generate the single header file
file(WRITE "${SINGLE_HEADER_PATH}" "// Automatically generated single header file\n")

foreach(HEADER ${execution_utility_HEADERS} ${engine_HEADERS} ${observer_HEADERS} ${controller_HEADERS})
  file(READ ${HEADER} HEADER_CONTENTS)
  string(REGEX REPLACE "#include \"[^\"]*\"\n" "" HEADER_CONTENTS "${HEADER_CONTENTS}")
  file(APPEND "${SINGLE_HEADER_PATH}" "${HEADER_CONTENTS}")
endforeach()

add_library(${PROJECT_NAME} STATIC ${execution_utility_SOURCES} ${engine_SOURCES} ${observer_SOURCES} ${controller_SOURCES})

if (DEBUG)
  target_compile_options(${PROJECT_NAME}
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
  )

  add_link_options(-fsanitize=address)

  target_link_options(${PROJECT_NAME}
    PRIVATE
    -fsanitize=address
  )
else()
  target_compile_options(${PROJECT_NAME}
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
endif()

# Install the header files
install(FILES ${SINGLE_HEADER_PATH} DESTINATION include)

# Install the library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)

