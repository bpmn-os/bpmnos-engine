cmake_minimum_required(VERSION 3.26.4)

# Get lists of all headers and sources
set(INSTANCE_READER_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")
include("${INSTANCE_READER_DIR}/CMakeLists.txt")
list(TRANSFORM data_HEADERS PREPEND "${INSTANCE_READER_DIR}/")
list(TRANSFORM data_SOURCES PREPEND "${INSTANCE_READER_DIR}/")

