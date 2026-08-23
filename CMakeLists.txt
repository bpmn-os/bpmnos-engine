cmake_minimum_required(VERSION 3.26.4)

project(bpmnos VERSION 0.1.0 LANGUAGES CXX)

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

# Docs-only: configure just the documentation, without building the engine (no cnl/limex/json fetch, no
# model/execution/app compilation, hence no bpmn++). The `docs` target runs Doxygen over the source and
# generates the BPMN-OS schema headers it needs with schematic++; cross-references to bpmn++ come from the
# downloaded tag file. Used by the GitHub Pages workflow to publish the docs without compiling anything.
option(BPMNOS_DOCS_ONLY "Configure only the documentation target (skip building the engine)" OFF)
if (BPMNOS_DOCS_ONLY)
  message("-- Docs-only configuration: engine build and its dependencies are skipped")
endif()

include_directories("${PROJECT_SOURCE_DIR}/")

# Build output goes to the build tree. Previously the archives and the amalgamated headers were written to
# lib/ in the checkout, which is why a consumer's include path named _deps/bpmnos-src/lib and why two build
# directories over one checkout overwrote each other's libraries. Both subprojects call project(), so they
# cannot name the engine's binary directory themselves; it is captured here.
set(BPMNOS_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(BPMNOS_LIB_DIR ${BPMNOS_BINARY_DIR}/lib)
set(BPMNOS_INCLUDE_DIR ${BPMNOS_BINARY_DIR}/include)
file(MAKE_DIRECTORY ${BPMNOS_LIB_DIR} ${BPMNOS_INCLUDE_DIR})

# The sources include the generated classes by a path relative to the project root, as
# "model/bpmnos/src/xml/bpmnos/...". They are generated under the same suffix in the build tree, so putting
# the binary directory on the include path resolves them without touching a hundred source files.
include_directories("${BPMNOS_BINARY_DIR}/")

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

# ccache is chosen by whoever is building, not imposed by a dependency. Set as a launcher rather than
# through set_property(GLOBAL RULE_LAUNCH_COMPILE), which model/ and execution/ used: that applied to those
# two directories' targets only, so a fetched dependency compiled without it, and it would have governed a
# consumer's targets had the engine been the dependency.
if (PROJECT_IS_TOP_LEVEL)
  find_program(CCACHE_FOUND "ccache")
  set(CCACHE_SUPPORT ON CACHE BOOL "Enable ccache support")
  if (CCACHE_FOUND AND CCACHE_SUPPORT)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_FOUND}")
  elseif (CCACHE_SUPPORT)
    message(WARNING
      "ccache not found. Re-compilation can be unnecessarily slow. "
      "To install run: sudo apt install ccache")
  endif()
endif()

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

if (NOT BPMNOS_DOCS_ONLY)

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

# LIMEX dependency. Pinned by tag: declaring it at main meant that pinning the engine did not determine the
# build, since a consumer fetching a given engine revision could get any later limex.
FetchContent_Declare(
        limex
        GIT_REPOSITORY https://github.com/bpmn-os/limex/
        GIT_TAG v0.1.0
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

# schematic++ generates the BPMN-OS extension classes from xsd/BPMNOS.xsd, and it has to be a runnable
# binary before this configure finishes rather than merely a target: the generator emits the CMakeLists
# listing its own output, and model/bpmnos/src/CMakeLists.txt reads that list while configuring. A
# FetchContent target would not be built until build time, which is too late, so a copy that cannot be
# resolved is built here in a sub-build of its own. bpmn++ declares schematic++ as well, for the same
# reason and for its own schemas.
set(SCHEMATICPP_VERSION 0.8.0)
# Names schematic++'s branch while its change is unmerged. It becomes v${SCHEMATICPP_VERSION} once tagged.
set(SCHEMATICPP_REF "dependency-management")
find_package(schematicpp ${SCHEMATICPP_VERSION} QUIET)

if(schematicpp_FOUND)
  get_target_property(SCHEMATICPP schematicpp::schematic++ IMPORTED_LOCATION_RELEASE)
  if(NOT SCHEMATICPP)
    get_target_property(SCHEMATICPP schematicpp::schematic++ IMPORTED_LOCATION)
  endif()
  message(STATUS "bpmnos: using an already-installed schematic++ at ${SCHEMATICPP}")
else()
  FetchContent_Declare(schematicpp
    GIT_REPOSITORY https://github.com/rajgoel/schematicpp.git
    GIT_TAG        ${SCHEMATICPP_REF}
    SOURCE_SUBDIR  do-not-add-this-subdirectory
  )
  FetchContent_MakeAvailable(schematicpp)

  set(SCHEMATICPP_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR}/schematicpp-host)
  message(STATUS "bpmnos: building schematic++ for the host in ${SCHEMATICPP_BUILD_DIR}")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -S ${schematicpp_SOURCE_DIR} -B ${SCHEMATICPP_BUILD_DIR}
            -DCMAKE_BUILD_TYPE=Release
    RESULT_VARIABLE r OUTPUT_QUIET
  )
  if(NOT r EQUAL 0)
    message(FATAL_ERROR "bpmnos: configuring schematic++ failed with ${r}")
  endif()
  execute_process(COMMAND ${CMAKE_COMMAND} --build ${SCHEMATICPP_BUILD_DIR} --parallel
                  RESULT_VARIABLE r OUTPUT_QUIET)
  if(NOT r EQUAL 0)
    message(FATAL_ERROR "bpmnos: building schematic++ failed with ${r}")
  endif()

  # schematic++ links its executable into its own source directory rather than its build directory.
  set(SCHEMATICPP ${schematicpp_SOURCE_DIR}/schematic++ CACHE FILEPATH "schematic++" FORCE)
  if(NOT EXISTS ${SCHEMATICPP})
    message(FATAL_ERROR "bpmnos: schematic++ was built but is not at ${SCHEMATICPP}")
  endif()
endif()

# bpmn++ parses the BPMN model. Declared rather than resolved from the system, so that a build succeeds on
# a machine with nothing installed. Xerces-C arrives as a usage requirement of its exported target.
set(BPMNPP_VERSION 0.1.0)
# Names bpmn++'s branch while its change is unmerged. It becomes v${BPMNPP_VERSION} once tagged.
set(BPMNPP_REF "dependency-management")
FetchContent_Declare(bpmnpp
  GIT_REPOSITORY https://github.com/bpmn-os/bpmnpp.git
  GIT_TAG        ${BPMNPP_REF}
  FIND_PACKAGE_ARGS ${BPMNPP_VERSION}
)
FetchContent_MakeAvailable(bpmnpp)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(BUILD_TESTING OFF CACHE BOOL "Disable testing")

add_subdirectory(model)
add_subdirectory(execution)
if(BPMNOS_TESTS)
  add_subdirectory(tests)
endif()
add_subdirectory(app)


# The engine is exported so that a consumer imports bpmnos::model and bpmnos::execution carrying their
# usage requirements, rather than reconstructing include paths into _deps.
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(EXPORT bpmnos-targets
  FILE      bpmnos-targets.cmake
  NAMESPACE bpmnos::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bpmnos
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/bpmnosConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/bpmnosConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bpmnos
)

# Below 1.0.0, so compatibility is SameMinorVersion: a consumer requiring 0.1.0 accepts any later 0.1.x
# and refuses 0.2.0.
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/bpmnosConfigVersion.cmake
  VERSION       ${PROJECT_VERSION}
  COMPATIBILITY SameMinorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/bpmnosConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/bpmnosConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bpmnos
)

# limex and nlohmann/json provide no package configuration, and the amalgamated headers include them
# directly, so their headers travel with the engine's own.
install(FILES ${limex_SOURCE_DIR}/limex.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(DIRECTORY ${json_SOURCE_DIR}/nlohmann DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

endif() # NOT BPMNOS_DOCS_ONLY

add_subdirectory(docs)

# Generate code profiles
if (NOT NDEBUG AND NOT BPMNOS_DOCS_ONLY)
  add_subdirectory(profile)
endif()


