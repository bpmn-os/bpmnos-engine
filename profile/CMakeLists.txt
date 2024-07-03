cmake_minimum_required(VERSION 3.26.4)

project(profiles)

# Set folder for the library to be created
message("-- Profiles will be created in folder: ${CMAKE_CURRENT_SOURCE_DIR}")

set(EXAMPLES_DIR "${CMAKE_SOURCE_DIR}/examples")

set(instances
  travelling_salesperson_problem/Travelling_salesperson_problem.bpmn
  assignment_problem/Assignment_problem.bpmn
  guided_knapsack_problem/Guided_knapsack_problem.bpmn
  guided_bin_packing_problem/Guided_bin_packing_problem.bpmn
  job_shop_scheduling_problem/Job_shop_scheduling_problem.bpmn
)

# Define a custom target to run profile.sh for each instance
add_custom_target(profiles)

# Iterate over each instance and add a custom command to run the script
foreach(instance ${instances})
  # Extract the directory part from the instance path
  get_filename_component(name ${instance} DIRECTORY)

  add_custom_command(
    TARGET profiles
    COMMAND ${CMAKE_COMMAND} -E echo "-- Create profile for ${name}"
#    COMMAND ${CMAKE_COMMAND} -E echo "./profile.sh -m ${EXAMPLES_DIR}/${instance} -d ${EXAMPLES_DIR}/${name}/instance.csv -p ${EXAMPLES_DIR}/${name} profile_${name}" 
    COMMAND ./profile.sh -m ${EXAMPLES_DIR}/${instance} -d ${EXAMPLES_DIR}/${name}/instance.csv -p ${EXAMPLES_DIR}/${name} profile_${name} 
    COMMAND ${CMAKE_COMMAND} -E copy profile_${name}.svg  ${CMAKE_SOURCE_DIR}/docs/images/
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )
endforeach()

add_custom_command(
  TARGET profiles
  COMMAND ${CMAKE_COMMAND} -E echo "Copy profiles to docs"
  COMMAND mkdir -p profiles
  COMMAND ${CMAKE_COMMAND} -E echo "# Code profiles" > profiles/main.md VERBATIM
  COMMAND ${CMAKE_COMMAND} -E echo "@page profiles Code profiles" >> profiles/main.md VERBATIM
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/md/
)

# Iterate over each instance and add link to profile
foreach(instance ${instances})
  # Extract the directory part from the instance path
  get_filename_component(name ${instance} DIRECTORY)
  string(REPLACE "_" " " model "${name}")
  add_custom_command(
    TARGET profiles
    COMMAND ${CMAKE_COMMAND} -E echo "- @subpage profile_${name}" >> profiles/main.md
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/md
  )
  add_custom_command(
    TARGET profiles
    COMMAND ${CMAKE_COMMAND} -E echo "# Code profile for the ${model}" > profiles/profile_${name}.md VERBATIM
    COMMAND ${CMAKE_COMMAND} -E echo "@page profile_${name} Code profile for the ${model}" >> profiles/profile_${name}.md VERBATIM
    COMMAND ${CMAKE_COMMAND} -E echo "This page contains a code profile for the  @ref ${name} model and this @subpage instance_${name} \"instance\"." >> profiles/profile_${name}.md VERBATIM    
    COMMAND ${CMAKE_COMMAND} -E echo "" >> profiles/profile_${name}.md VERBATIM    
    COMMAND ${CMAKE_COMMAND} -E echo "![Code profiling graph](profile_${name}.svg)" >> profiles/profile_${name}.md VERBATIM    
    COMMAND ${CMAKE_COMMAND} -E echo "# Instance" > profiles/instance_${name}.md VERBATIM
    COMMAND ${CMAKE_COMMAND} -E echo "@page instance_${name} Instance" >> profiles/instance_${name}.md VERBATIM
    COMMAND ${CMAKE_COMMAND} -E echo "@include ${name}/instance.csv" >> profiles/instance_${name}.md VERBATIM    
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/md
  )
endforeach()

message("-- To include code profiles in documentation run: make docs")
