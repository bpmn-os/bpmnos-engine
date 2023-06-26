cmake_minimum_required(VERSION 3.26.4)

project(bpmnos-model)

# Check dependencies
find_library(XERCES_LIB xerces-c)
if(NOT XERCES_LIB)
  message(FATAL_ERROR "xerces-c library not found.\nTo install run: sudo apt install libxerces-c-dev")
endif()

find_program(SCHEMATICPP schematic++)
if(NOT SCHEMATICPP)
  message(FATAL_ERROR "schematic++ not found.\nDownload schematic++ from: https://github.com/rajgoel/schematicpp")
endif()

# Set folder for the library to be created
set(LIB_DIR "lib" CACHE STRING "Library directory")
message("-- Library will be created in folder: ${LIB_DIR}")

# Set folder in which sources are located
set(SOURCE_DIR "${PROJECT_SOURCE_DIR}/src" CACHE STRING "Source directory")
set(XML_DIR "${SOURCE_DIR}/xml" CACHE STRING "XML classes source directory")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/${LIB_DIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

find_program(CCACHE_FOUND "ccache")
set(CCACHE_SUPPORT ON CACHE BOOL "Enable ccache support")
if (CCACHE_FOUND AND CCACHE_SUPPORT)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "ccache")
endif()

set(SCHEMATICPP_DIR ${CMAKE_CURRENT_BINARY_DIR}/schematicpp)
set(SCHEMATICPP_OUTPUT_DIR ${SCHEMATICPP_DIR}/xml)

if(NOT EXISTS "${XML_DIR}")
  message("-- Create XML classes in: ${XML_DIR}")
  file(MAKE_DIRECTORY ${XML_DIR})
  # Generate XML classes and headers
  execute_process(
    COMMAND ${SCHEMATICPP} -n bpmnos -o ${XML_DIR} -i xsd/BPMNOS.xsd
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endif()

# Generate the single header file
set(SINGLE_HEADER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${LIB_DIR}/${PROJECT_NAME}.h")
message("-- Header location: ${SINGLE_HEADER_PATH}")

# Check if 'bpmn++.h' already exists
message("-- Create single header file")

# Create output directory for the library
file(MAKE_DIRECTORY ${LIB_DIR})

# Create lists of all headers
#set(HEADERS "${XML_DIR}/XMLObject.h")
set(HEADERS "")
include("${XML_DIR}/bpmnos/CMakeLists.txt")
foreach(HEADER ${bpmnos_HEADERS})
  list(APPEND HEADERS "${XML_DIR}/${HEADER}")
endforeach()
list(APPEND HEADERS "${SOURCE_DIR}/SequenceFlow.h")
list(APPEND HEADERS "${SOURCE_DIR}/Node.h")
list(APPEND HEADERS "${SOURCE_DIR}/Model.h")

# Generate the single header file
file(WRITE "${SINGLE_HEADER_PATH}" "// Automatically generated single header file\n")

foreach(HEADER ${HEADERS})
  file(READ ${HEADER} HEADER_CONTENTS)
  string(REGEX REPLACE "#include \".*\"\n" "" HEADER_CONTENTS "${HEADER_CONTENTS}")
  file(APPEND "${SINGLE_HEADER_PATH}" "${HEADER_CONTENTS}")
endforeach()

# Generate the documentation
find_program(DOXYGEN_FOUND "doxygen")
set(DOCUMENTATION ON CACHE BOOL "Create documentation")
if (DOXYGEN_FOUND AND DOCUMENTATION)
  # Add Doxygen target
  add_custom_target(docs
    COMMAND doxygen ${PROJECT_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating Doxygen documentation (this may take a while)"
    VERBATIM
  )
endif()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

# Create list of all sources for the library
#set(SOURCES "${XML_DIR}/XMLObject.cpp")
set(SOURCES "")
include("${XML_DIR}/bpmnos/CMakeLists.txt")
foreach(SOURCE ${bpmnos_SOURCES})
  list(APPEND SOURCES "${XML_DIR}/${SOURCE}")
endforeach()
list(APPEND SOURCES "${SOURCE_DIR}/SequenceFlow.cpp")
list(APPEND SOURCES "${SOURCE_DIR}/Node.cpp")
list(APPEND SOURCES "${SOURCE_DIR}/Model.cpp")

add_library(${PROJECT_NAME} STATIC ${SOURCES})

# Set optimization flags for the target based on the compiler
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  target_compile_options(${PROJECT_NAME} PRIVATE -O3)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_options(${PROJECT_NAME} PRIVATE -O3)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE /Ox)
endif()

# Install the header files
install(FILES ${SINGLE_HEADER_PATH} DESTINATION include)

# Install the library
install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib)

