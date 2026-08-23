# bpmn++, schematic++ and Xerces-C are no longer resolved from the system here. bpmn++ and schematic++ are
# declared at the top level, and Xerces-C arrives as a usage requirement of bpmnpp::bpmn++: no hand-written
# code in this project uses it, only the classes generated below, which derive from bpmn++'s XMLObject.

# Set folder in which sources are located
# Not cache variables: bpmn++ declares SOURCE_DIR and XML_DIR in the cache too, and set(... CACHE ...)
# does not overwrite an existing entry, so whichever project configures first would decide both.
set(SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")

# The generated classes are build output and belong in the build tree, not in the checkout.
set(XML_DIR "${BPMNOS_BINARY_DIR}/model/bpmnos/src/xml")

# Regenerated on every configure rather than once: guarding this on the directory's existence meant that
# editing xsd/BPMNOS.xsd changed nothing until someone deleted the directory by hand, and a stale copy
# survived every rebuild and every change of pin.
set(BPMNOS_SCHEMA ${CMAKE_CURRENT_LIST_DIR}/xsd/BPMNOS.xsd)

message("-- Create XML classes in: ${XML_DIR}")
file(MAKE_DIRECTORY ${XML_DIR})
# The schema is named relatively, from a working directory: schematic++ copies the path it is given into a
# comment in every generated class, so an absolute path would put the builder's home directory into the
# amalgamated header.
execute_process(
  COMMAND ${SCHEMATICPP} -n bpmnos -o ${XML_DIR} -i xsd/BPMNOS.xsd
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  RESULT_VARIABLE BPMNOS_GENERATE_RESULT
  OUTPUT_QUIET
)
if(NOT BPMNOS_GENERATE_RESULT EQUAL 0)
  message(FATAL_ERROR "bpmnos: schematic++ failed with ${BPMNOS_GENERATE_RESULT}")
endif()

# Editing the schema forces a reconfigure, which is what regenerates the classes. Generation cannot be a
# build rule, because the list of sources it produces is read while configuring.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${BPMNOS_SCHEMA})

# Get lists of all headers and sources
# src/CMakeLists.txt resolves both sets itself: the generated ones against XML_DIR in the build tree and
# the hand-written ones against SOURCE_DIR, which no longer share a prefix.
include("${SOURCE_DIR}/CMakeLists.txt")


