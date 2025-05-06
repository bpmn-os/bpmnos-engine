cmake_minimum_required(VERSION 3.26.4)

# Set folder in which sources are located
set(EXECUTIONMODEL_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")

# Get lists of all headers and sources
include("${EXECUTIONMODEL_SOURCE_DIR}/CMakeLists.txt")

list(TRANSFORM executionmodel_HEADERS PREPEND "${EXECUTIONMODEL_SOURCE_DIR}/")
list(TRANSFORM executionmodel_SOURCES PREPEND "${EXECUTIONMODEL_SOURCE_DIR}/")

