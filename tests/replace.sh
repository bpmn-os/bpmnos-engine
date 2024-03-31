#!/bin/bash
#exit
for i in $(find -name '*.bpmn'); do 
  echo $i
  
  old='xmlns:execution="https://bpmn.telematique.eu/execution"'
  new='xmlns:bpmnos="https://bpmnos.telematique.eu"'
  echo $old " -> " $new
  rpl $old $new $i

  old='execution:'
  new='bpmnos:'
  echo $old " -> " $new
  rpl $old $new $i

  old='type="xs:'
  new='type="'
  echo $old " -> " $new
  rpl $old $new $i
done
