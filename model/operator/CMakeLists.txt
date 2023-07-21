cmake_minimum_required(VERSION 3.26.4)

# Get lists of all headers and sources
set(OPERATOR_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")
include("${OPERATOR_DIR}/CMakeLists.txt")
list(TRANSFORM operator_HEADERS PREPEND "${OPERATOR_DIR}/")
list(TRANSFORM operator_SOURCES PREPEND "${OPERATOR_DIR}/")

