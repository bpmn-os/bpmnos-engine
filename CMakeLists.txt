cmake_minimum_required(VERSION 3.26.4)

project(bpmnos)

option(NDEBUG "Whether compilation uses address sanitizer without optimization" ON)
if (NDEBUG)
  message("-- Compiler optimizations turned on")
else()
  message("-- Compiler optimizations turned off and address sanitizer used")
endif()

include_directories("${PROJECT_SOURCE_DIR}/")

# Set the C++ standard
set(CMAKE_CXX_STANDARD 23)

include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

#  Compositional Numeric Library
FetchContent_Declare(
        cnl
        GIT_REPOSITORY https://github.com/johnmcfarlane/cnl.git
        GIT_TAG v1.1.2
)

if (NOT cnl_POPULATED)
    FetchContent_Populate(cnl)
endif ()
include_directories(
  SYSTEM
  ${cnl_SOURCE_DIR}/include 
)

# CSV Parser
FetchContent_Declare(
        csv
#        GIT_REPOSITORY https://github.com/vincentlaucsb/csv-parser.git
        GIT_REPOSITORY https://github.com/rajgoel/csv-parser.git
#        GIT_TAG 2.2.3
)

if (NOT csv_POPULATED)
    FetchContent_Populate(csv)
endif ()
include_directories(
  SYSTEM
  ${csv_SOURCE_DIR}/single_include 
)

# ExprTK dependency
FetchContent_Declare(
        exprtk
        GIT_REPOSITORY https://github.com/ArashPartow/exprtk/
        GIT_TAG 0.0.1
)

FetchContent_GetProperties(exprtk)
if (NOT exprtk_POPULATED)
    FetchContent_Populate(exprtk)
    add_library(exprtk INTERFACE)
    # by using the SYSTEM flag the compiler ignores any warnings of the library
endif ()
include_directories(
  SYSTEM INTERFACE 
  ${exprtk_SOURCE_DIR}
)
#target_link_libraries(${PROJECT_NAME} exprtk)

# nlohmann/json dependency
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.2
        GIT_PROGRESS   TRUE
)

FetchContent_GetProperties(json)
if (NOT json_POPULATED)
    set(JSON_BuildTests OFF)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR})
    #    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif ()
include_directories(
  SYSTEM
  ${json_SOURCE_DIR}/single_include 
)
#target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

add_subdirectory(model)
add_subdirectory(execution)
add_subdirectory(tests)
add_subdirectory(app)

# Generate the documentation
set(DOCUMENTATION ON CACHE BOOL "Create documentation")
if (DOCUMENTATION)
  find_program(BPMN2SVG_FOUND "bpmn2svg")
  if (NOT BPMN2SVG_FOUND)
    message(FATAL_ERROR "bpmn2svg not found.\n\nDownload bpmn2svg from: https://github.com/bpmn-os/bpmnos-modeller")
  endif()
  find_program(BPMNDOC_FOUND "bpmndoc")
  if (NOT BPMNDOC_FOUND)
    message(FATAL_ERROR "bpmndoc not found.\n\nDownload bpmndoc from: https://github.com/bpmn-os/bpmndoc")
  endif()
  find_program(DOXYGEN_FOUND "doxygen")
  if (NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "doxygen not found.\nTo install run: sudo apt install doxygen")
  endif()
  find_program(DOT_FOUND "dot")
  if (NOT DOT_FOUND)
    message(FATAL_ERROR "dot not found.\nTo install run: sudo apt install graphviz")
  endif()
#  find_program(DRAWIO_FOUND "drawio")
#  if (NOT DRAWIO_FOUND)
#    message(FATAL_ERROR "drawio not found.\nTo install run: sudo snap install drawio")
#  endif()
  include(ExternalProject)

  ExternalProject_Add(doxygen-awesome
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG v2.3.1
        SOURCE_DIR ${CMAKE_BINARY_DIR}/doxygen-awesome
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        CONFIGURE_COMMAND ""
  )

  add_subdirectory(docs)
  set(MODEL_SRC ${PROJECT_SOURCE_DIR}/examples)
  set(MODEL_DOC ${PROJECT_SOURCE_DIR}/docs/md/models)
  set(HTML_DIR ${PROJECT_SOURCE_DIR}/docs/html)
  add_custom_target(bpmn2svg
    COMMAND bash -c "echo 'Convert bpmn to svg ...'"
    COMMAND bash -c "for model in ${MODEL_SRC}/*/*bpmn; do baseName=$(basename $model '.bpmn'); folder=$(basename $(dirname $model)); if [[ $model -nt \"$folder/$baseName.svg\" ]]; then bpmn2svg $model -o $folder; fi; done"
    COMMENT "Generating BPMN model diagrams (this may take a while)"
    WORKING_DIRECTORY ${MODEL_DOC}
    VERBATIM
  )

  add_custom_target(bpmndoc_md
    COMMAND bash -c "mkdir -p ${HTML_DIR}"
    COMMAND bash -c "echo 'Convert bpmn to md ...'"
    COMMAND bash -c "for model in ${MODEL_SRC}/*/*bpmn; do baseName=$(basename $model '.bpmn'); folder=$(basename $(dirname $model)); cd $folder; bpmndoc $model; cp *.svg  ${HTML_DIR}; cd ..; done"
    WORKING_DIRECTORY ${MODEL_DOC}
    COMMENT "Generating BPMN model documentation"
    VERBATIM
    DEPENDS bpmn2svg
  )

add_custom_target(bpmndoc
  COMMAND bash -c "IFS=';' read -ra pages <<< '${MODEL_PAGES}'; for page in \"\${pages[@]}\"; do filename=\$(echo \$page | cut -d' ' -f1); title=\$(echo \$page | cut -d' ' -f2-); sed 's|!\\[\\([^ ]*\\)](\\([^ ]*\\.svg\\))|\\\\htmlonly\\n<object data=\"\\2\" type=\"image\\/svg+xml\" style=\"max-width: 100%;\">\\1<\\/object>\\n\\\\endhtmlonly\\n|g' \$filename > temp_file && mv temp_file \$filename; echo \"$(echo \"$title\"; cat $filename)\" > $filename; done"
  WORKING_DIRECTORY ${MODEL_DOC}
  COMMENT "Generating BPMN model pages"
  VERBATIM
  DEPENDS bpmndoc_md
)

  # Configure Doxyfile
  find_path(BPMNPP_INCLUDE_PATH bpmn++.h)
  # Add Doxygen target
  add_custom_target(docs
    COMMAND bash -c "sed -i 's|https://github.com/jothepro/doxygen-awesome-css|https://github.com/bpmn-os/bpmnos-engine|g' ${CMAKE_BINARY_DIR}/doxygen-awesome/doxygen-custom/header.html"
    COMMAND bash -c "echo '<script type=\"module\">import mermaid from \"https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs\"; mermaid.initialize({ startOnLoad: true, theme: \"neutral\" });</script></body></html>' > ${CMAKE_BINARY_DIR}/doxygen-awesome/doxygen-custom/footer.html"
    COMMAND ${CMAKE_COMMAND} -E env "DOXYGEN_AWESOME_DIR=${CMAKE_BINARY_DIR}/doxygen-awesome" "BPMNPP_INCLUDE_PATH=${BPMNPP_INCLUDE_PATH}" doxygen Doxyfile
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating Doxygen documentation (this may take a while)"
    VERBATIM
    DEPENDS doxygen-awesome bpmndoc
  )
endif()


