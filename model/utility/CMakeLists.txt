cmake_minimum_required(VERSION 3.26.4)

# Get lists of all headers and sources
set(UTILITY_READER_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")
include("${UTILITY_READER_DIR}/CMakeLists.txt")
list(TRANSFORM utility_HEADERS PREPEND "${UTILITY_READER_DIR}/")
list(TRANSFORM utility_SOURCES PREPEND "${UTILITY_READER_DIR}/")

