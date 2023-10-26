cmake_minimum_required(VERSION 3.26.4)

# Set folder in which sources are located
set(CONTROLLER_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")

# Get lists of all headers and sources
include("${CONTROLLER_SOURCE_DIR}/CMakeLists.txt")

list(TRANSFORM controller_HEADERS PREPEND "${CONTROLLER_SOURCE_DIR}/")
list(TRANSFORM controller_SOURCES PREPEND "${CONTROLLER_SOURCE_DIR}/")

