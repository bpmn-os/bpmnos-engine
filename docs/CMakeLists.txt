set(MODEL_PAGES
    "earliest_arrival_problem/Earliest_arrival_problem.md @page earliest_arrival_problem Earliest arrival problem"
    "assignment_problem/Assignment_problem.md @page assignment_problem Assignment problem"
    "knapsack_problem/Knapsack_problem.md @page knapsack_problem Knapsack problem"
    "guided_knapsack_problem/Guided_knapsack_problem.md @page guided_knapsack_problem Knapsack problem (guided)"
    "bin_packing_problem/Bin_packing_problem.md @page bin_packing_problem Bin packing problem"
    "guided_bin_packing_problem/Guided_bin_packing_problem.md @page guided_bin_packing_problem Bin packing problem (guided)"
    "job_shop_scheduling_problem/Job_shop_scheduling_problem.md @page job_shop_scheduling_problem Job shop scheduling problem"
    "vehicle_routing_problem/Vehicle_routing_problem.md @page vehicle_routing_problem Vehicle routing problem"
    "guided_vehicle_routing_problem/Guided_vehicle_routing_problem.md @page guided_vehicle_routing_problem Vehicle routing problem (guided)"
    "pickup_delivery_problem/Pickup_delivery_problem.md @page pickup_delivery_problem Pickup and delivery problem"
    "guided_pickup_delivery_problem/Guided_pickup_delivery_problem.md @page guided_pickup_delivery_problem Pickup and delivery problem (guided)"
    "travelling_salesperson_problem/Travelling_salesperson_problem.md @page travelling_salesperson_problem Travelling salesperson problem"
    "truck_driver_scheduling_problem/US_Truck_driver_scheduling_problem.md @page truck_driver_scheduling_problem Truck driver scheduling problem"
)

# Generate the documentation
set(DOCUMENTATION ON CACHE BOOL "Create documentation")
if (DOCUMENTATION)
  find_program(BPMNOSDOC_FOUND "bpmnosdoc")
  if (NOT BPMNOSDOC_FOUND)
    message(FATAL_ERROR "bpmnosdoc not found.\n\nInstall it from https://github.com/bpmn-os/bpmnos-js (npm install; npm link)")
  endif()
  find_program(SCHEMATICPP "schematic++")
  if (NOT SCHEMATICPP)
    message(FATAL_ERROR "schematic++ not found.\nDownload schematic++ from: https://github.com/bpmn-os/schematicpp")
  endif()
  find_program(DOXYGEN_FOUND "doxygen")
  if (NOT DOXYGEN_FOUND)
    message(FATAL_ERROR "doxygen not found.\nTo install run: sudo apt install doxygen")
  endif()
  find_program(DOT_FOUND "dot")
  if (NOT DOT_FOUND)
    message(FATAL_ERROR "dot not found.\nTo install run: sudo apt install graphviz")
  endif()
  include(ExternalProject)

  ExternalProject_Add(doxygen-awesome
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG v2.3.4
        SOURCE_DIR ${CMAKE_BINARY_DIR}/doxygen-awesome
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        CONFIGURE_COMMAND ""
  )

  
  # BPMN-OS extension classes: schematic++ generates them from the XSD into model/bpmnos/src/xml. A full
  # build does this in model/bpmnos; for a docs-only build (engine not configured) generate them here, as
  # they are part of the Doxygen INPUT. Only schematic++ is needed, not bpmn++.
  set(BPMNOS_XML_DIR ${PROJECT_SOURCE_DIR}/model/bpmnos/src/xml)
  if (NOT EXISTS "${BPMNOS_XML_DIR}")
    message("-- Generating BPMN-OS XML classes in: ${BPMNOS_XML_DIR}")
    file(MAKE_DIRECTORY ${BPMNOS_XML_DIR})
    execute_process(
      COMMAND ${SCHEMATICPP} -n bpmnos -o ${BPMNOS_XML_DIR} -i xsd/BPMNOS.xsd
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/model/bpmnos
    )
  endif()

  set(MODEL_SRC ${PROJECT_SOURCE_DIR}/examples)
  set(MODEL_DOC ${PROJECT_SOURCE_DIR}/docs/md/models)
  set(HTML_DIR ${PROJECT_SOURCE_DIR}/docs/html)
  # bpmnosdoc (bpmn-os/bpmnos-js) renders each example model to SVG and writes a markdown page of its
  # execution data — replacing the former bpmnos2svg + bpmndoc pair, and with it the bpmn++ dependency.
  add_custom_target(model_docs
    COMMAND bash -c "mkdir -p ${HTML_DIR}"
    COMMAND bash -c "for model in ${MODEL_SRC}/*/*bpmn; do folder=$(basename $(dirname $model)); bpmnosdoc \"$model\" -o \"${MODEL_DOC}/$folder\"; cp \"${MODEL_DOC}/$folder/\"*.svg ${HTML_DIR}; done"
    COMMENT "Generating BPMN-OS model documentation (bpmnosdoc: markdown + diagrams)"
    WORKING_DIRECTORY ${MODEL_DOC}
    VERBATIM
  )

  # Assemble the Doxygen pages from bpmnosdoc's markdown: turn each `![alt](x.svg)` into an inline SVG
  # object (alt may contain spaces, e.g. "Diagram: <sub-process>"), and prepend the `@page id title`.
  add_custom_target(model_pages
    COMMAND bash -c "IFS=';' read -ra pages <<< '${MODEL_PAGES}'; for page in \"\${pages[@]}\"; do filename=\$(echo \$page | cut -d' ' -f1); title=\$(echo \$page | cut -d' ' -f2-); sed 's|!\\[\\([^]]*\\)](\\([^ ]*\\.svg\\))|\\\\htmlonly\\n<object data=\"\\2\" type=\"image\\/svg+xml\" style=\"max-width: 100%;\">\\1<\\/object>\\n\\\\endhtmlonly\\n|g' \$filename > temp_file && mv temp_file \$filename; echo \"$(echo \"$title\"; cat $filename)\" > $filename; done"
    WORKING_DIRECTORY ${MODEL_DOC}
    COMMENT "Assembling BPMN-OS model pages"
    VERBATIM
    DEPENDS model_docs
  )

  # Configure Doxyfile
  find_path(BPMNPP_INCLUDE_PATH bpmn++.h)
  # Add Doxygen target
  add_custom_target(docs
    COMMAND bash -c "sed -i 's|https://github.com/jothepro/doxygen-awesome-css|https://github.com/bpmn-os/bpmnos-engine|g' ${CMAKE_BINARY_DIR}/doxygen-awesome/doxygen-custom/header.html"
    COMMAND bash -c "echo '<script type=\"module\">import mermaid from \"https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs\"; mermaid.initialize({ startOnLoad: true, theme: \"neutral\" });</script></body></html>' > ${CMAKE_BINARY_DIR}/doxygen-awesome/doxygen-custom/footer.html"
    COMMAND bash -c "curl -s -o ${CMAKE_BINARY_DIR}/bpmnpp.tag https://bpmn-os.github.io/bpmnpp/bpmnpp.tag"
    COMMAND bash -c "DOXYGEN_AWESOME_DIR=${CMAKE_BINARY_DIR}/doxygen-awesome BPMNPP_INCLUDE_PATH=${BPMNPP_INCLUDE_PATH} BPMNPP_DOCS_PATH=${CMAKE_BINARY_DIR} doxygen docs/Doxyfile 2>&1 | grep -v 'Unsupported xml/html tag <choice>' || true"
    COMMAND bash -c "sed -i 's|$(\"body\").css({overflow: \"hidden\"});|//$(\"body\").css({overflow: \"hidden\"});|g' docs/html/resize.js"
# TODO: resize.js: // $("body").css({overflow: "hidden"});
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating Doxygen documentation (this may take a while)"
    VERBATIM
    DEPENDS doxygen-awesome model_pages
  )

endif()

