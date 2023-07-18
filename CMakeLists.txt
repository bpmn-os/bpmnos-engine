cmake_minimum_required(VERSION 3.26.4)

project(bpmnos)

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

# Define the clean target
#add_custom_target(clean
#    COMMAND ${CMAKE_COMMAND} -E echo "Cleaning..."
#    COMMENT "Cleaning"
#)

add_subdirectory(model)
add_subdirectory(engine)
#add_subdirectory(controller)

# Generate the documentation
find_program(DOXYGEN_FOUND "doxygen")
set(DOCUMENTATION ON CACHE BOOL "Create documentation")
if (DOXYGEN_FOUND AND DOCUMENTATION)
  # Configure Doxyfile
  find_path(BPMNPP_INCLUDE_PATH bpmn++.h)
  # Add Doxygen target
  add_custom_target(docs
    COMMAND doxygen ${PROJECT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating Doxygen documentation (this may take a while)"
    VERBATIM
  )
endif()


