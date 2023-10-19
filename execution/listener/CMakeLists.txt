cmake_minimum_required(VERSION 3.26.4)

# Set folder in which sources are located
set(LISTENER_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")

# Get lists of all headers and sources
include("${LISTENER_SOURCE_DIR}/CMakeLists.txt")

list(TRANSFORM listener_HEADERS PREPEND "${LISTENER_SOURCE_DIR}/")
list(TRANSFORM listener_SOURCES PREPEND "${LISTENER_SOURCE_DIR}/")

