cmake_minimum_required(VERSION 3.26.4)

project(bpmnos)

include_directories("${PROJECT_SOURCE_DIR}/include")
add_subdirectory(model/parser)

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

