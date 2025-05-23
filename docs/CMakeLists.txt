set(MODEL_PAGES
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
  find_program(BPMN2SVG_FOUND "bpmn2svg")
  if (NOT BPMN2SVG_FOUND)
    message(FATAL_ERROR "bpmn2svg not found.\n\nDownload bpmn2svg from: https://github.com/bpmn-os/bpmnos-modeller" and run npm install; sudo npm link)
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
  include(ExternalProject)

  ExternalProject_Add(doxygen-awesome
        GIT_REPOSITORY https://github.com/jothepro/doxygen-awesome-css.git
        GIT_TAG v2.3.4
        SOURCE_DIR ${CMAKE_BINARY_DIR}/doxygen-awesome
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        CONFIGURE_COMMAND ""
  )

  
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
    COMMAND ${CMAKE_COMMAND} -E env "DOXYGEN_AWESOME_DIR=${CMAKE_BINARY_DIR}/doxygen-awesome" "BPMNPP_INCLUDE_PATH=${BPMNPP_INCLUDE_PATH}" doxygen docs/Doxyfile
    COMMAND bash -c "sed -i 's|$(\"body\").css({overflow: \"hidden\"});|//$(\"body\").css({overflow: \"hidden\"});|g' docs/html/resize.js"
# TODO: resize.js: // $("body").css({overflow: "hidden"});
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating Doxygen documentation (this may take a while)"
    VERBATIM
    DEPENDS doxygen-awesome bpmndoc
  )

endif()

