include("${SOURCE_DIR}/xml/bpmnos/CMakeLists.txt")

set(bpmn_HEADERS ${bpmnos_HEADERS})
set(bpmn_SOURCES ${bpmnos_SOURCES})
list(TRANSFORM bpmn_HEADERS PREPEND "xml/")
list(TRANSFORM bpmn_SOURCES PREPEND "xml/")

list(APPEND bpmn_SOURCES
  extensionElements/Attribute.cpp
  extensionElements/Parameter.cpp
  extensionElements/Expression.cpp
  extensionElements/expression/LinearExpression.cpp
  extensionElements/expression/GenericExpression.cpp
  extensionElements/expression/NullCondition.cpp
  extensionElements/expression/Enumeration.cpp
  extensionElements/expression/StringExpression.cpp
  extensionElements/Restriction.cpp
  extensionElements/Operator.cpp
  extensionElements/operator/UnassignOperator.cpp
  extensionElements/operator/AssignOperator.cpp
  extensionElements/operator/LookupTable.cpp
  extensionElements/operator/LookupOperator.cpp
  extensionElements/operator/ExpressionOperator.cpp
  extensionElements/Choice.cpp
  extensionElements/Guidance.cpp
  extensionElements/Content.cpp
  extensionElements/MessageDefinition.cpp
  extensionElements/ExtensionElements.cpp
  extensionElements/Gatekeeper.cpp
  extensionElements/Timer.cpp
  extensionElements/Data.cpp
  DecisionTask.cpp
  SequentialAdHocSubProcess.cpp
  Model.cpp
)

list(APPEND bpmn_HEADERS
  extensionElements/Attribute.h
  extensionElements/Parameter.h
  extensionElements/Expression.h
  extensionElements/expression/LinearExpression.h
  extensionElements/expression/GenericExpression.h
  extensionElements/expression/NullCondition.h
  extensionElements/expression/Enumeration.h
  extensionElements/expression/StringExpression.h
  extensionElements/Restriction.h
  extensionElements/Operator.h
  extensionElements/operator/UnassignOperator.h
  extensionElements/operator/AssignOperator.h
  extensionElements/operator/LookupTable.h
  extensionElements/operator/LookupOperator.h
  extensionElements/operator/ExpressionOperator.h
  extensionElements/Choice.h
  extensionElements/Guidance.h
  extensionElements/Content.h
  extensionElements/MessageDefinition.h
  extensionElements/ExtensionElements.h
  extensionElements/Gatekeeper.h
  extensionElements/Timer.h
  extensionElements/Data.h
  DecisionTask.h
  SequentialAdHocSubProcess.h
  Model.h
)
