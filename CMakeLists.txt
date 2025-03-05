cmake_minimum_required(VERSION 3.26.4)

project(bpmnos)

option(NDEBUG "Whether compilation uses optimization" ON)
if (NDEBUG)
  message("-- Compiler optimizations turned on")
else()
  message("-- Compiler optimizations turned off and address sanitizer used")
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

# nlohmann/json dependency
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
        GIT_PROGRESS   TRUE
        SOURCE_SUBDIR this-directory-does-not-exist # Work around to not build the project
)

FetchContent_GetProperties(json)
FetchContent_MakeAvailable(json)

include_directories(
  SYSTEM
  ${json_SOURCE_DIR}/single_include 
)
#target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(BUILD_TESTING OFF CACHE BOOL "Disable testing")
# String Util
FetchContent_Declare(
        strutil
        GIT_REPOSITORY https://github.com/Shot511/strutil.git
        GIT_TAG v1.1.0
        SOURCE_SUBDIR this-directory-does-not-exist # Work around to not build the project
)

FetchContent_MakeAvailable(strutil)

include_directories(
  SYSTEM
  ${strutil_SOURCE_DIR} 
)

# CP
FetchContent_Declare(
        cp
        GIT_REPOSITORY https://github.com/bpmn-os/cp.git
        GIT_TAG main
)

FetchContent_GetProperties(cp)
FetchContent_MakeAvailable(cp)

include_directories(
  SYSTEM
  ${cp_SOURCE_DIR}
)

add_subdirectory(model)
add_subdirectory(execution)
add_subdirectory(tests)
add_subdirectory(app)
add_subdirectory(docs)

# Generate code profiles
if (NOT NDEBUG)
  add_subdirectory(profile)
endif()


