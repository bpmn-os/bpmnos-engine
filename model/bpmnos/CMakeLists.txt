# Check dependencies
find_library(XERCES_LIB xerces-c)
if(NOT XERCES_LIB)
  message(FATAL_ERROR "xerces-c library not found.\nTo install run: sudo apt install libxerces-c-dev")
endif()

find_program(SCHEMATICPP schematic++)
if(NOT SCHEMATICPP)
  message(FATAL_ERROR "schematic++ not found.\nDownload schematic++ from: https://github.com/rajgoel/schematicpp")
endif()


# Set folder in which sources are located
set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")
set(XML_DIR "${SOURCE_DIR}/xml" CACHE STRING "XML classes source directory")

set(SCHEMATICPP_DIR ${CMAKE_CURRENT_BINARY_DIR}/schematicpp)
set(SCHEMATICPP_OUTPUT_DIR ${SCHEMATICPP_DIR}/xml)

if(NOT EXISTS "${XML_DIR}")
  message("-- Create XML classes in: ${XML_DIR}")
  file(MAKE_DIRECTORY ${XML_DIR})
  # Generate XML classes and headers
  execute_process(
    COMMAND ${SCHEMATICPP} -n bpmnos -o ${XML_DIR} -i xsd/BPMNOS.xsd
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bpmnos
  )
endif()

# Get lists of all headers and sources
include("${SOURCE_DIR}/CMakeLists.txt")
list(TRANSFORM bpmn_HEADERS PREPEND "${SOURCE_DIR}/")
list(TRANSFORM bpmn_SOURCES PREPEND "${SOURCE_DIR}/")


