# Code profiles are a developer tool, not documentation: each one measures one machine at one moment, and
# reproducing it needs an instrumented build and a run of every example. They are therefore written to the
# build tree and no longer copied into the checkout or turned into Doxygen pages. Open them with a browser,
# for instance `xdg-open build/profile`.

if (NOT TARGET bpmnos-greedy)
  message(FATAL_ERROR "bpmnos: BPMNOS_PROFILE requires the application; configure with -DBPMNOS_APP=ON")
endif()

set(EXAMPLES_DIR ${PROJECT_SOURCE_DIR}/examples)
message("-- Profiles will be created in folder: ${CMAKE_BINARY_DIR}")
message("-- Profiles will be created from example folder: ${EXAMPLES_DIR}")

set(instances
  travelling_salesperson_problem/Travelling_salesperson_problem.bpmn
  assignment_problem/Assignment_problem.bpmn
  knapsack_problem/Knapsack_problem.bpmn
  bin_packing_problem/Bin_packing_problem.bpmn
  job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn
)

# The instrumented executable must exist before the script runs it, which a POST_BUILD command on a target
# without dependencies did not ensure: every profile was taken of a binary that was not there.
add_custom_target(profiles)
add_dependencies(profiles bpmnos-greedy)

foreach(instance ${instances})
  get_filename_component(name ${instance} DIRECTORY)

  add_custom_command(
    TARGET profiles
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "-- Create profile for ${name}"
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/profile.sh
            -x $<TARGET_FILE:bpmnos-greedy>
            -m ${EXAMPLES_DIR}/${instance}
            -d ${EXAMPLES_DIR}/${name}/instance.csv
            -p ${EXAMPLES_DIR}/${name}
            profile_${name}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endforeach()
