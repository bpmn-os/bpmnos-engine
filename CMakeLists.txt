cmake_minimum_required(VERSION 3.26.4)

project(bpmnos)

option(NDEBUG "Whether compilation uses optimization" ON)
if (NDEBUG)
  message("-- Compiler optimizations turned on")
else()
  message("-- Compiler optimizations turned off and address sanitizer used")
endif()

option(BPMNOS_TESTS "Build bpmnos tests" ON)
if (BPMNOS_TESTS)
  message("-- Tests are turned on")
else()
  message("-- Tests are turned off")
endif()

include_directories("${PROJECT_SOURCE_DIR}/")

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

#  Compositional Numeric Library
FetchContent_Declare(
        cnl
        GIT_REPOSITORY https://github.com/johnmcfarlane/cnl.git
        GIT_TAG v1.1.2
        SOURCE_SUBDIR this-directory-does-not-exist # Work around to not build the project
)

FetchContent_MakeAvailable(cnl)

include_directories(
  SYSTEM
  ${cnl_SOURCE_DIR}/include 
)

# LIMEX dependency
FetchContent_Declare(
        limex
        GIT_REPOSITORY https://github.com/bpmn-os/limex/
        GIT_TAG main
)

FetchContent_GetProperties(limex)
FetchContent_MakeAvailable(limex)

include_directories(
  SYSTEM INTERFACE 
  ${limex_SOURCE_DIR}
)

# nlohmann/json dependency - download single header file
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
        DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_MakeAvailable(json)

# Create nlohmann directory structure for the header
file(MAKE_DIRECTORY ${json_SOURCE_DIR}/nlohmann)
file(COPY_FILE ${json_SOURCE_DIR}/json.hpp ${json_SOURCE_DIR}/nlohmann/json.hpp)

include_directories(
  SYSTEM
  ${json_SOURCE_DIR}
)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(BUILD_TESTING OFF CACHE BOOL "Disable testing")

add_subdirectory(model)
add_subdirectory(execution)
if(BPMNOS_TESTS)
  add_subdirectory(tests)
endif()
add_subdirectory(app)
add_subdirectory(docs)

# Generate code profiles
if (NOT NDEBUG)
  add_subdirectory(profile)
endif()


