cmake_minimum_required(VERSION 3.26.4)

# Set folder in which sources are located
set(EXECUTION_DATA_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")

# Get lists of all headers and sources
include("${EXECUTION_DATA_SOURCE_DIR}/CMakeLists.txt")

list(TRANSFORM execution_data_HEADERS PREPEND "${EXECUTION_DATA_SOURCE_DIR}/")
list(TRANSFORM execution_data_SOURCES PREPEND "${EXECUTION_DATA_SOURCE_DIR}/")
