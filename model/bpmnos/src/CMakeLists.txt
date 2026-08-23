# The generated classes live in XML_DIR, which is in the build tree, while the hand-written sources live in
# SOURCE_DIR, so the two sets no longer share a prefix and are resolved separately.
include("${XML_DIR}/bpmnos/CMakeLists.txt")

set(bpmn_xml_HEADERS ${bpmnos_HEADERS})
set(bpmn_xml_SOURCES ${bpmnos_SOURCES})
list(TRANSFORM bpmn_xml_HEADERS PREPEND "${XML_DIR}/")
list(TRANSFORM bpmn_xml_SOURCES PREPEND "${XML_DIR}/")

set(bpmn_SOURCES
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

set(bpmn_HEADERS
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

# The hand-written sources are relative and resolved against SOURCE_DIR; the xml set goes first, as it did
# when both were relative, since the amalgamated header is these files concatenated in list order.
list(TRANSFORM bpmn_SOURCES PREPEND "${SOURCE_DIR}/")
list(TRANSFORM bpmn_HEADERS PREPEND "${SOURCE_DIR}/")
list(PREPEND bpmn_SOURCES ${bpmn_xml_SOURCES})
list(PREPEND bpmn_HEADERS ${bpmn_xml_HEADERS})
