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
# An absent renderer disables the documentation rather than the whole configure step. Refusing to
# configure the engine because an optional target's tooling is missing stopped every consumer that did not
# know to pass -DDOCUMENTATION=OFF.
if (BPMNOS_DOCS)
  find_program(DOXYGEN_FOUND "doxygen")
  find_program(DOT_FOUND "dot")
  if (NOT DOXYGEN_FOUND OR NOT DOT_FOUND)
    message(WARNING "doxygen or graphviz not found; no docs target. "
                    "To install run: sudo apt install doxygen graphviz")
    set(BPMNOS_DOCS OFF)
  endif()
endif()

# bpmnosdoc renders the example models into the markdown the Doxygen pages are assembled from. Without it
# the documentation is still built, without those pages.
if (BPMNOS_DOCS)
  find_program(BPMNOSDOC_FOUND "bpmnosdoc")
  if (NOT BPMNOSDOC_FOUND)
    message(WARNING "bpmnosdoc not found; model pages will be missing from the documentation. "
                    "Install it from https://github.com/bpmn-os/bpmnos-js (npm install; npm link)")
  endif()
endif()

if (BPMNOS_DOCS)
  include(ExternalProject)

  ExternalProject_Add(doxygen-awesome
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG v2.3.4
        SOURCE_DIR ${CMAKE_BINARY_DIR}/doxygen-awesome
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        CONFIGURE_COMMAND ""
  )

  
  # The extension classes are part of the Doxygen INPUT. A full build generates them into the build tree in
  # model/bpmnos; a docs-only build does not configure the engine, so they are generated here instead.
  if (BPMNOS_DOCS_ONLY)
    set(BPMNOS_XML_DIR ${CMAKE_CURRENT_BINARY_DIR}/xml)
    message("-- Generating BPMN-OS XML classes in: ${BPMNOS_XML_DIR}")
    file(MAKE_DIRECTORY ${BPMNOS_XML_DIR})
    execute_process(
      COMMAND ${SCHEMATICPP} -n bpmnos -o ${BPMNOS_XML_DIR} -i xsd/BPMNOS.xsd
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/model/bpmnos
    )
  else()
    set(BPMNOS_XML_DIR ${XML_DIR})
  endif()

  # Documentation is build output like everything else: the markdown bpmnosdoc renders, the pages assembled
  # from it and the Doxygen html all go to the build tree rather than into the checkout.
  set(MODEL_SRC ${PROJECT_SOURCE_DIR}/examples)
  set(BPMNOS_DOCS_DIR ${CMAKE_CURRENT_BINARY_DIR})
  set(MODEL_DOC ${BPMNOS_DOCS_DIR}/md/models)
  set(HTML_DIR ${BPMNOS_DOCS_DIR}/html)
  file(MAKE_DIRECTORY ${MODEL_DOC} ${HTML_DIR})
  # bpmnosdoc (bpmn-os/bpmnos-js) renders each example model to SVG and writes a markdown page of its
  # execution data — replacing the former bpmnos2svg + bpmndoc pair, and with it the bpmn++ dependency.
  add_custom_target(model_docs
    COMMAND bash -c "mkdir -p ${HTML_DIR}"
    COMMAND bash -c "for model in ${MODEL_SRC}/*/*bpmn; do folder=$(basename $(dirname $model)); bpmnosdoc \"$model\" -o \"${MODEL_DOC}/$folder\"; cp \"${MODEL_DOC}/$folder/\"*.svg ${HTML_DIR}; done"
    COMMENT "Generating BPMN-OS model documentation (bpmnosdoc: markdown + diagrams)"
    WORKING_DIRECTORY ${MODEL_DOC}
    VERBATIM
  )

  # The model pages exist only when bpmnosdoc rendered them; without it the documentation is built without.
  if (BPMNOSDOC_FOUND)
    set(BPMNOS_MODEL_PAGES_DEPENDENCY model_pages)
  endif()

  # Assemble the Doxygen pages from bpmnosdoc's markdown: turn each `![alt](x.svg)` into an inline SVG
  # object (alt may contain spaces, e.g. "Diagram: <sub-process>"), and prepend the `@page id title`.
  add_custom_target(model_pages
    COMMAND bash -c "IFS=';' read -ra pages <<< '${MODEL_PAGES}'; for page in \"\${pages[@]}\"; do filename=\$(echo \$page | cut -d' ' -f1); title=\$(echo \$page | cut -d' ' -f2-); sed 's|!\\[\\([^]]*\\)](\\([^ ]*\\.svg\\))|\\\\htmlonly\\n<object data=\"\\2\" type=\"image\\/svg+xml\" style=\"max-width: 100%;\">\\1<\\/object>\\n\\\\endhtmlonly\\n|g' \$filename > temp_file && mv temp_file \$filename; echo \"$(echo \"$title\"; cat $filename)\" > $filename; done"
    WORKING_DIRECTORY ${MODEL_DOC}
    COMMENT "Assembling BPMN-OS model pages"
    VERBATIM
    DEPENDS model_docs
  )

  # bpmn++'s header is cross-referenced by the documentation. It is no longer installed on the system, so
  # its location comes from where the engine's build put it rather than from a search.
  if (TARGET bpmn++)
    set(BPMNPP_INCLUDE_PATH ${bpmnpp_BINARY_DIR}/include)
  else()
    find_path(BPMNPP_INCLUDE_PATH bpmn++.h)
  endif()
  # Add Doxygen target
  add_custom_target(docs
    COMMAND bash -c "sed -i 's|https://github.com/jothepro/doxygen-awesome-css|https://github.com/bpmn-os/bpmnos-engine|g' ${CMAKE_BINARY_DIR}/doxygen-awesome/doxygen-custom/header.html"
    COMMAND bash -c "echo '<script type=\"module\">import mermaid from \"https://cdn.jsdelivr.net/npm/mermaid@10/dist/mermaid.esm.min.mjs\"; mermaid.initialize({ startOnLoad: true, theme: \"neutral\" });</script></body></html>' > ${CMAKE_BINARY_DIR}/doxygen-awesome/doxygen-custom/footer.html"
    COMMAND bash -c "curl -s -o ${CMAKE_BINARY_DIR}/bpmnpp.tag https://bpmn-os.github.io/bpmnpp/bpmnpp.tag"
    COMMAND bash -c "DOXYGEN_AWESOME_DIR=${CMAKE_BINARY_DIR}/doxygen-awesome BPMNPP_INCLUDE_PATH=${BPMNPP_INCLUDE_PATH} BPMNOS_XML_DIR=${BPMNOS_XML_DIR} BPMNOS_DOCS_DIR=${BPMNOS_DOCS_DIR} BPMNOS_MODEL_DOC=${MODEL_DOC} BPMNPP_DOCS_PATH=${CMAKE_BINARY_DIR} doxygen docs/Doxyfile 2>&1 | grep -v 'Unsupported xml/html tag <choice>' || true"
    COMMAND bash -c "sed -i 's|$(\"body\").css({overflow: \"hidden\"});|//$(\"body\").css({overflow: \"hidden\"});|g' ${HTML_DIR}/resize.js"
# TODO: resize.js: // $("body").css({overflow: "hidden"});
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating Doxygen documentation (this may take a while)"
    VERBATIM
    DEPENDS doxygen-awesome ${BPMNOS_MODEL_PAGES_DEPENDENCY}
  )

endif()

