include("${SOURCE_DIR}/xml/bpmnos/CMakeLists.txt")

list(TRANSFORM bpmnos_HEADERS PREPEND "xml/")
list(TRANSFORM bpmnos_SOURCES PREPEND "xml/")

list(APPEND bpmnos_SOURCES
  Status.cpp
  Gatekeeper.cpp
  Model.cpp
#  RequestActivity.cpp
#  ReleaseActivity.cpp
#  ResourceActivity.cpp
#  Job.cpp
# Timer.cpp: tParameter trigger
# Recipient.cpp: tMessage message
# Sender.cpp: tMessage message
)

list(APPEND bpmnos_HEADERS
  Status.h
  Gatekeeper.h
  Model.h
#  RequestActivity.h
#  ReleaseActivity.h
#  ResourceActivity.h
#  Job.h
)
