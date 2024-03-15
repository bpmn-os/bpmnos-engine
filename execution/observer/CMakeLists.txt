cmake_minimum_required(VERSION 3.26.4)

# Set folder in which sources are located
set(OBSERVER_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")

# Get lists of all headers and sources
include("${OBSERVER_SOURCE_DIR}/CMakeLists.txt")

list(TRANSFORM observer_HEADERS PREPEND "${OBSERVER_SOURCE_DIR}/")
list(TRANSFORM observer_SOURCES PREPEND "${OBSERVER_SOURCE_DIR}/")

