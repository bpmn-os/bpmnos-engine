list(APPEND engine_SOURCES
  Message.cpp
  Token.cpp
  Event.cpp
  events/TerminationEvent.cpp
  events/ClockTickEvent.cpp
  events/ReadyEvent.cpp
  events/EntryEvent.cpp
  events/CompletionEvent.cpp
  events/TaskCompletionEvent.cpp
#  events/TriggerEvent.cpp
#  events/TimerEvent.cpp
#  events/MessageDeliveryEvent.cpp
  events/ExitEvent.cpp
  StateMachine.cpp
  SystemState.cpp
  EventHandler.cpp
  eventhandler/TimeWarp.cpp
  eventhandler/Metronome.cpp
  eventhandler/InstantExitHandler.cpp
  eventhandler/InstantEntryHandler.cpp
  eventhandler/DeterministicTaskCompletionHandler.cpp
#  eventhandler/TimerHandler.cpp
  eventhandler/ReadyHandler.cpp
  Engine.cpp
)

list(APPEND engine_HEADERS
  Message.h
  Token.h
  Event.h
  events/TerminationEvent.h
  events/ClockTickEvent.h
  events/ReadyEvent.h
  events/EntryEvent.h
  events/CompletionEvent.h
  events/TaskCompletionEvent.h
  events/ChoiceEvent.h
  events/ResourceShutdownEvent.h
#  events/TriggerEvent.h
#  events/TimerEvent.h
#  events/MessageDeliveryEvent.h
  events/ExitEvent.h
  StateMachine.h
  SystemState.h
  EventHandler.h
  eventhandler/TimeWarp.h
  eventhandler/Metronome.h
  eventhandler/InstantExitHandler.h
  eventhandler/InstantEntryHandler.h
  eventhandler/DeterministicTaskCompletionHandler.h
#  eventhandler/TimerHandler.h
  eventhandler/ReadyHandler.h
  Engine.h
)
