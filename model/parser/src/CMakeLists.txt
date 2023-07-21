include("${SOURCE_DIR}/xml/bpmnos/CMakeLists.txt")

set(bpmn_HEADERS ${bpmnos_HEADERS})
set(bpmn_SOURCES ${bpmnos_SOURCES})
list(TRANSFORM bpmn_HEADERS PREPEND "xml/")
list(TRANSFORM bpmn_SOURCES PREPEND "xml/")

list(APPEND bpmn_SOURCES
  Attribute.cpp
  Parameter.cpp
  Restriction.cpp
  Operator.cpp
  Status.cpp
  Gatekeeper.cpp
  Model.cpp
  DecisionTask.cpp
  JobShop.cpp
  ResourceActivity.cpp
  RequestActivity.cpp
  ReleaseActivity.cpp
  MessageTaskSubstitution.cpp
# Timer.cpp: tParameter trigger
# Recipient.cpp: tMessage message
# Sender.cpp: tMessage message
)

list(APPEND bpmn_HEADERS
  Attribute.h
  Parameter.h
  Restriction.h
  Operator.h
  Status.h
  Gatekeeper.h
  Model.h
  DecisionTask.h
  JobShop.h
  ResourceActivity.h
  RequestActivity.h
  ReleaseActivity.h
  MessageTaskSubstitution.h
)
