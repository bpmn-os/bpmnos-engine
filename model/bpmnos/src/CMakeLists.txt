include("${SOURCE_DIR}/xml/bpmnos/CMakeLists.txt")

set(bpmn_HEADERS ${bpmnos_HEADERS})
set(bpmn_SOURCES ${bpmnos_SOURCES})
list(TRANSFORM bpmn_HEADERS PREPEND "xml/")
list(TRANSFORM bpmn_SOURCES PREPEND "xml/")

list(APPEND bpmn_SOURCES
  extensionElements/Attribute.cpp
  extensionElements/AttributeRegistry.cpp
  extensionElements/Parameter.cpp
  extensionElements/Expression.cpp
  extensionElements/Restriction.cpp
  extensionElements/Operator.cpp
  extensionElements/Choice.cpp
  extensionElements/Guidance.cpp
  extensionElements/Content.cpp
  extensionElements/MessageDefinition.cpp
  extensionElements/ExtensionElements.cpp
  extensionElements/Gatekeeper.cpp
  extensionElements/Timer.cpp
  extensionElements/Signal.cpp
  extensionElements/Conditions.cpp
  DecisionTask.cpp
  SequentialAdHocSubProcess.cpp
  LookupTable.cpp
  Model.cpp
)

list(APPEND bpmn_HEADERS
  extensionElements/Attribute.h
  extensionElements/AttributeRegistry.h
  extensionElements/Parameter.h
  extensionElements/Expression.h
  extensionElements/Restriction.h
  extensionElements/Operator.h
  extensionElements/Choice.h
  extensionElements/Guidance.h
  extensionElements/Content.h
  extensionElements/MessageDefinition.h
  extensionElements/ExtensionElements.h
  extensionElements/Gatekeeper.h
  extensionElements/Timer.h
  extensionElements/Signal.h
  extensionElements/Conditions.h
  DecisionTask.h
  SequentialAdHocSubProcess.h
  LookupTable.h
  Model.h
)
