cmake_minimum_required(VERSION 3.26.4)

# Get lists of all headers and sources
set(EXECUTION_UTILITY_READER_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")
include("${EXECUTION_UTILITY_READER_DIR}/CMakeLists.txt")
list(TRANSFORM execution_utility_HEADERS PREPEND "${EXECUTION_UTILITY_READER_DIR}/")
list(TRANSFORM execution_utility_SOURCES PREPEND "${EXECUTION_UTILITY_READER_DIR}/")

