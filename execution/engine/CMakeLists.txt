cmake_minimum_required(VERSION 3.26.4)

# Set folder in which sources are located
set(ENGINE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")

# Get lists of all headers and sources
include("${ENGINE_SOURCE_DIR}/CMakeLists.txt")

list(TRANSFORM engine_HEADERS PREPEND "${ENGINE_SOURCE_DIR}/")
list(TRANSFORM engine_SOURCES PREPEND "${ENGINE_SOURCE_DIR}/")

