cmake_minimum_required(VERSION 3.26.4)

project(bpmnos-engine)

# Set folder for the library to be created
set(LIB_DIR "lib" CACHE STRING "Library directory")
message("-- Library will be created in folder: ${CMAKE_SOURCE_DIR}/${LIB_DIR}")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/${LIB_DIR})
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

find_program(CCACHE_FOUND "ccache")
set(CCACHE_SUPPORT ON CACHE BOOL "Enable ccache support")
if (CCACHE_FOUND AND CCACHE_SUPPORT)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK "ccache")
endif()

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)

add_library(${PROJECT_NAME} STATIC 
  src/Engine.cpp
  src/Token.cpp
)

target_compile_options(${PROJECT_NAME}
  PRIVATE
  -O3
  -Werror
  -Wno-error=unused-parameter
  -Wno-error=unused-variable
  -pedantic-errors
  -Wpedantic
  -Wall
  -Wextra
  -Wconversion
  -Wsign-conversion
)

