cmake_minimum_required(VERSION 3.26.4)

# Get lists of all headers and sources
set(MODEL_UTILITY_READER_DIR "${CMAKE_CURRENT_LIST_DIR}/src" CACHE STRING "Source directory")
include("${MODEL_UTILITY_READER_DIR}/CMakeLists.txt")
list(TRANSFORM model_utility_HEADERS PREPEND "${MODEL_UTILITY_READER_DIR}/")
list(TRANSFORM model_utility_SOURCES PREPEND "${MODEL_UTILITY_READER_DIR}/")

