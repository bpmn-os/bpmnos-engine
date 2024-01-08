cmake_minimum_required(VERSION 3.26.4)

project(bpmnos)

option(DEBUG "Whether compilation uses address sanitizer without optimization" ON)
if (DEBUG)
  message("-- Compiler optimizations turned off and address sanitizer used")
else()
  message("-- Compiler optimizations turned on")
endif()

include_directories("${PROJECT_SOURCE_DIR}/")

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
        GIT_TAG 2.1.3
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
#add_subdirectory(controller)
add_subdirectory(tests)

# Generate the documentation
set(DOCUMENTATION ON CACHE BOOL "Create documentation")
if (DOCUMENTATION)
  find_program(DOXYGEN_FOUND "doxygen")
  if (NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "doxygen not found.\nTo install run: sudo apt install doxygen")
  endif()
  find_program(DOT_FOUND "dot")
  if (NOT DOT_FOUND)
    message(FATAL_ERROR "dot not found.\nTo install run: sudo apt install graphviz")
  endif()
  find_program(DRAWIO_FOUND "drawio")
  if (NOT DRAWIO_FOUND)
    message(FATAL_ERROR "drawio not found.\nTo install run: sudo snap install drawio")
  endif()
  include(ExternalProject)

  ExternalProject_Add(doxygen-awesome
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG v2.3.1
        SOURCE_DIR ${CMAKE_BINARY_DIR}/doxygen-awesome
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        CONFIGURE_COMMAND ""
  )

  # Configure Doxyfile
  find_path(BPMNPP_INCLUDE_PATH bpmn++.h)
  # Add Doxygen target
  add_custom_target(docs
    COMMAND bash -c "drawio -xt --crop execution/engine/Token_life-cycle.drawio -o docs/images/Token_life-cycle.pdf"
    COMMAND bash -c "drawio -xt execution/engine/Token_life-cycle.drawio -o docs/images/Token_life-cycle.png"
    COMMAND bash -c "sed -i 's|https://github.com/jothepro/doxygen-awesome-css|https://github.com/bpmn-os/bpmnos-engine|g' ${CMAKE_BINARY_DIR}/doxygen-awesome/doxygen-custom/header.html"
    COMMAND bash -c "echo '</body></html>' > ${CMAKE_BINARY_DIR}/doxygen-awesome/doxygen-custom/footer.html"
    COMMAND ${CMAKE_COMMAND} -E env "DOXYGEN_AWESOME_DIR=${CMAKE_BINARY_DIR}/doxygen-awesome" "BPMNPP_INCLUDE_PATH=${BPMNPP_INCLUDE_PATH}" doxygen Doxyfile
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating Doxygen documentation (this may take a while)"
    VERBATIM
    DEPENDS doxygen-awesome
  )
endif()
