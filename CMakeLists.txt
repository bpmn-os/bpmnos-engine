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
)

if (NOT cnl_POPULATED)
    FetchContent_Populate(cnl)
endif ()
include_directories(
  SYSTEM
  ${cnl_SOURCE_DIR}/include 
)

# CSV Parser
FetchContent_Declare(
        csv
        GIT_REPOSITORY https://github.com/vincentlaucsb/csv-parser.git
        GIT_TAG 2.3.0
)

if (NOT csv_POPULATED)
    FetchContent_Populate(csv)
endif ()
include_directories(
  SYSTEM
  ${csv_SOURCE_DIR}/single_include 
)

# ExprTK dependency
FetchContent_Declare(
        exprtk
        GIT_REPOSITORY https://github.com/ArashPartow/exprtk/
        GIT_TAG 0.0.1
)

FetchContent_GetProperties(exprtk)
if (NOT exprtk_POPULATED)
    FetchContent_Populate(exprtk)
    add_library(exprtk INTERFACE)
    # by using the SYSTEM flag the compiler ignores any warnings of the library
endif ()
include_directories(
  SYSTEM INTERFACE 
  ${exprtk_SOURCE_DIR}
)
#target_link_libraries(${PROJECT_NAME} exprtk)

# nlohmann/json dependency
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
        GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(json)
if (NOT json_POPULATED)
    set(JSON_BuildTests OFF)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR})
    #    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif ()
include_directories(
  SYSTEM
  ${json_SOURCE_DIR}/single_include 
)
#target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

add_subdirectory(model)
add_subdirectory(execution)
add_subdirectory(tests)
add_subdirectory(app)
add_subdirectory(docs)

# Generate code profiles
if (NOT NDEBUG)
  add_subdirectory(profile)
endif()


