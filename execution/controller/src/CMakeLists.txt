list(APPEND controller_SOURCES
  dispatcher/TimeWarp.cpp
  dispatcher/Metronome.cpp
  dispatcher/ReadyHandler.cpp
  dispatcher/InstantEntry.cpp
  dispatcher/FirstComeFirstServedSequentialEntry.cpp
  dispatcher/BestFirstSequentialEntry.cpp
  dispatcher/DeterministicTaskCompletion.cpp
  dispatcher/InstantExit.cpp
  dispatcher/FirstMatchingMessageDelivery.cpp
  dispatcher/RandomChoice.cpp
  dispatcher/MyopicDecisionTaskTerminator.cpp
  dispatcher/MyopicMessageTaskTerminator.cpp
  Controller.cpp
  GreedyController.cpp
)

list(APPEND controller_HEADERS
  dispatcher/TimeWarp.h
  dispatcher/Metronome.h
  dispatcher/ReadyHandler.h
  dispatcher/InstantEntry.h
  dispatcher/FirstComeFirstServedSequentialEntry.h
  dispatcher/BestFirstSequentialEntry.h
  dispatcher/DeterministicTaskCompletion.h
  dispatcher/InstantExit.h
  dispatcher/FirstMatchingMessageDelivery.h
  dispatcher/RandomChoice.h
  dispatcher/MyopicDecisionTaskTerminator.h
  dispatcher/MyopicMessageTaskTerminator.h
  Controller.h
  GreedyController.h
)
