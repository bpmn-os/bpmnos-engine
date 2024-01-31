include("${SOURCE_DIR}/xml/bpmnos/CMakeLists.txt")

set(bpmn_HEADERS ${bpmnos_HEADERS})
set(bpmn_SOURCES ${bpmnos_SOURCES})
list(TRANSFORM bpmn_HEADERS PREPEND "xml/")
list(TRANSFORM bpmn_SOURCES PREPEND "xml/")

list(APPEND bpmn_SOURCES
  extensionElements/Attribute.cpp
  extensionElements/Parameter.cpp
  extensionElements/Restriction.cpp
  extensionElements/Operator.cpp
  extensionElements/operators/Unset.cpp
  extensionElements/operators/Set.cpp
  extensionElements/operators/LookupTable.cpp
  extensionElements/operators/Lookup.cpp
  extensionElements/operators/Expression.cpp
  extensionElements/operators/LinearExpression.cpp
  extensionElements/operators/GenericExpression.cpp
  extensionElements/Decision.cpp
  extensionElements/Guidance.cpp
  extensionElements/Status.cpp
  extensionElements/Gatekeeper.cpp
  extensionElements/Timer.cpp
  extensionElements/Content.cpp
  extensionElements/Message.cpp
  extensionElements/MessageStatus.cpp
  DecisionTask.cpp
  Sequencer.cpp
  ResourceActivity.cpp
  RequestActivity.cpp
  ReleaseActivity.cpp
  MessageTaskSubstitution.cpp
  Model.cpp
)

list(APPEND bpmn_HEADERS
  extensionElements/Attribute.h
  extensionElements/Parameter.h
  extensionElements/Restriction.h
  extensionElements/Operator.h
  extensionElements/operators/Unset.h
  extensionElements/operators/Set.h
  extensionElements/operators/LookupTable.h
  extensionElements/operators/Lookup.h
  extensionElements/operators/Expression.h
  extensionElements/operators/LinearExpression.h
  extensionElements/operators/GenericExpression.h
  extensionElements/Decision.h
  extensionElements/Guidance.h
  extensionElements/Status.h
  extensionElements/Gatekeeper.h
  extensionElements/Timer.h
  extensionElements/Content.h
  extensionElements/Message.h
  extensionElements/MessageStatus.h
  DecisionTask.h
  Sequencer.h
  ResourceActivity.h
  RequestActivity.h
  ReleaseActivity.h
  MessageTaskSubstitution.h
  Model.h
)
