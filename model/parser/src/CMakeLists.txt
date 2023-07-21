include("${SOURCE_DIR}/xml/bpmnos/CMakeLists.txt")

set(bpmn_HEADERS ${bpmnos_HEADERS})
set(bpmn_SOURCES ${bpmnos_SOURCES})
list(TRANSFORM bpmn_HEADERS PREPEND "xml/")
list(TRANSFORM bpmn_SOURCES PREPEND "xml/")

list(APPEND bpmn_SOURCES
  extensionElements/Attribute.cpp
  extensionElements/Parameter.cpp
  extensionElements/Restriction.cpp
  extensionElements/operators/Unset.cpp
  extensionElements/operators/Set.cpp
  extensionElements/operators/Lookup.cpp
  extensionElements/operators/LookupTable.cpp
  extensionElements/operators/Expression.cpp
  extensionElements/Operator.cpp
  extensionElements/Status.cpp
  extensionElements/Gatekeeper.cpp
# extensionElements/Timer.cpp: tParameter trigger
# extensionElements/MessageContent.cpp: tMessage message
# extensionElements/Decisions.cpp: tDecision decision
  Model.cpp
  DecisionTask.cpp
  JobShop.cpp
  ResourceActivity.cpp
  RequestActivity.cpp
  ReleaseActivity.cpp
  MessageTaskSubstitution.cpp
)

list(APPEND bpmn_HEADERS
  extensionElements/Attribute.h
  extensionElements/Parameter.h
  extensionElements/Restriction.h
  extensionElements/operators/Unset.h
  extensionElements/operators/Set.h
  extensionElements/operators/Lookup.h
  extensionElements/operators/LookupTable.h
  extensionElements/operators/Expression.h
  extensionElements/Operator.h
  extensionElements/Status.h
  extensionElements/Gatekeeper.h
  Model.h
  DecisionTask.h
  JobShop.h
  ResourceActivity.h
  RequestActivity.h
  ReleaseActivity.h
  MessageTaskSubstitution.h
)
