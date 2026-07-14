cmake_minimum_required(VERSION 3.26.4)

project(bpmnos)

option(NDEBUG "Whether compilation uses optimization" ON)
if (NDEBUG)
  message("-- Compiler optimizations turned on")
else()
  message("-- Compiler optimizations turned off and address sanitizer used")
endif()

option(BPMNOS_TESTS "Build bpmnos tests" ON)
if (BPMNOS_TESTS)
  message("-- Tests are turned on")
else()
  message("-- Tests are turned off")
endif()

include_directories("${PROJECT_SOURCE_DIR}/")

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

# Compiler-specific spellings of otherwise-common warning/diagnostic flags.
# Used verbatim in the target_compile_options blocks of the sub-projects.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(BPMNOS_SHADOW_FLAG -Wshadow=local)
  set(BPMNOS_MAX_ERRORS_FLAG -fmax-errors=1) # Remove
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # clang has no equivalent of -Wshadow=local: its "shadows a field" and
  # "shadows a local" diagnostics are both under -Wshadow and cannot be
  # separated. Since the project deliberately allows parameters/locals to
  # shadow member fields (e.g. the ubiquitous status/data/globals parameters),
  # no shadow flag is passed for clang; the GCC build still enforces
  # -Wshadow=local for local-vs-local shadowing.
  set(BPMNOS_SHADOW_FLAG "")
  set(BPMNOS_MAX_ERRORS_FLAG -ferror-limit=1) # Remove
endif()

# Additional warnings accepted by both GCC and clang. Enabled for parity so
# that either compiler surfaces missing overrides, hidden virtual overloads,
# non-virtual destructors of polymorphic bases, and struct/class tag mismatches.
set(BPMNOS_WARNING_FLAGS
  -Wsuggest-override
  -Woverloaded-virtual
  -Wnon-virtual-dtor
  -Wmismatched-tags
)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

#  Compositional Numeric Library
FetchContent_Declare(
        cnl
        GIT_REPOSITORY https://github.com/johnmcfarlane/cnl.git
        GIT_TAG v1.1.2
        SOURCE_SUBDIR this-directory-does-not-exist # Work around to not build the project
)

FetchContent_MakeAvailable(cnl)

include_directories(
  SYSTEM
  ${cnl_SOURCE_DIR}/include 
)

# LIMEX dependency
FetchContent_Declare(
        limex
        GIT_REPOSITORY https://github.com/bpmn-os/limex/
        GIT_TAG main
)

FetchContent_GetProperties(limex)
FetchContent_MakeAvailable(limex)

include_directories(
  SYSTEM INTERFACE 
  ${limex_SOURCE_DIR}
)

# nlohmann/json dependency - download single header file
FetchContent_Declare(
        json
        URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.hpp
        DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_MakeAvailable(json)

# Create nlohmann directory structure for the header
file(MAKE_DIRECTORY ${json_SOURCE_DIR}/nlohmann)
file(COPY_FILE ${json_SOURCE_DIR}/json.hpp ${json_SOURCE_DIR}/nlohmann/json.hpp)

include_directories(
  SYSTEM
  ${json_SOURCE_DIR}
)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(BUILD_TESTING OFF CACHE BOOL "Disable testing")

add_subdirectory(model)
add_subdirectory(execution)
if(BPMNOS_TESTS)
  add_subdirectory(tests)
endif()
add_subdirectory(app)
add_subdirectory(docs)

# Generate code profiles
if (NOT NDEBUG)
  add_subdirectory(profile)
endif()


