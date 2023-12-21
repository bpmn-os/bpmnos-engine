cmake_minimum_required(VERSION 3.26.4)

project(bpmnos-model)

# Set folder for the library to be created
set(LIB_DIR "lib" CACHE STRING "Library directory")
message("-- Library will be created in folder: ${CMAKE_SOURCE_DIR}/${LIB_DIR}")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${LIB_DIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# String Util
FetchContent_Declare(
        strutil
        GIT_REPOSITORY https://github.com/Shot511/strutil.git
        GIT_TAG dafb13e8e5e7761dc9a731862a666221e82cd546 # Version 1.0.2 as of Jan 7, 2023
)

if (NOT strutil_POPULATED)
    FetchContent_Populate(strutil)
endif ()

include_directories(
  SYSTEM
  ${strutil_SOURCE_DIR} 
)

# Get lists of all headers and sources of utilities
include("${PROJECT_SOURCE_DIR}/utility/CMakeLists.txt")

# Get lists of all headers and sources of BPMN parser
include("${PROJECT_SOURCE_DIR}/parser/CMakeLists.txt")

# Get lists of all headers and sources of data reader
include("${PROJECT_SOURCE_DIR}/data/CMakeLists.txt")

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

foreach(HEADER ${model_utility_HEADERS} ${bpmn_HEADERS} ${data_HEADERS})
  file(READ ${HEADER} HEADER_CONTENTS)
  string(REGEX REPLACE "#include \"[^\"]*\"\n" "" HEADER_CONTENTS "${HEADER_CONTENTS}")
  file(APPEND "${SINGLE_HEADER_PATH}" "${HEADER_CONTENTS}")
endforeach()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} STATIC ${model_utility_SOURCES} ${bpmn_SOURCES} ${data_SOURCES})

# Delete xml directory when running `make clean`
set_property(
  TARGET ${PROJECT_NAME}
  APPEND
  PROPERTY ADDITIONAL_CLEAN_FILES "${XML_DIR}"
)

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
  message("Compiling with optimizations")
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
#install(FILES ${SINGLE_HEADER_PATH} DESTINATION include)

# Install the library
#install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)

