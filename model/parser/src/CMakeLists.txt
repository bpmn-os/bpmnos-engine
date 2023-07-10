include("${SOURCE_DIR}/xml/bpmnos/CMakeLists.txt")

list(TRANSFORM bpmnos_HEADERS PREPEND "xml/")
list(TRANSFORM bpmnos_SOURCES PREPEND "xml/")

list(APPEND bpmnos_SOURCES
  Status.cpp
  Gatekeeper.cpp
  Model.cpp
  JobShop.cpp
  ResourceActivity.cpp
  RequestActivity.cpp
  ReleaseActivity.cpp
  MessageTaskSubstitution.cpp
# Timer.cpp: tParameter trigger
# Recipient.cpp: tMessage message
# Sender.cpp: tMessage message
)

list(APPEND bpmnos_HEADERS
  Status.h
  Gatekeeper.h
  Model.h
  JobShop.h
  ResourceActivity.h
  RequestActivity.h
  ReleaseActivity.h
  MessageTaskSubstitution.h
)
