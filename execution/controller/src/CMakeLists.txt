list(APPEND controller_SOURCES
  dispatcher/TimeWarp.cpp
  dispatcher/Metronome.cpp
  dispatcher/ReadyHandler.cpp
  dispatcher/DeterministicTaskCompletion.cpp
  dispatcher/naive/InstantEntry.cpp
  dispatcher/naive/FirstComeFirstServedSequentialEntry.cpp
  dispatcher/naive/InstantExit.cpp
  dispatcher/naive/FirstMatchingMessageDelivery.cpp
  dispatcher/naive/RandomChoice.cpp
  dispatcher/naive/MyopicDecisionTaskTerminator.cpp
  dispatcher/naive/MyopicMessageTaskTerminator.cpp
  dispatcher/greedy/BestFirstSequentialEntry.cpp
  Controller.cpp
  GreedyController.cpp
)

list(APPEND controller_HEADERS
  dispatcher/TimeWarp.h
  dispatcher/Metronome.h
  dispatcher/ReadyHandler.h
  dispatcher/DeterministicTaskCompletion.h
  dispatcher/naive/InstantEntry.h
  dispatcher/naive/FirstComeFirstServedSequentialEntry.h
  dispatcher/naive/InstantExit.h
  dispatcher/naive/FirstMatchingMessageDelivery.h
  dispatcher/naive/RandomChoice.h
  dispatcher/naive/MyopicDecisionTaskTerminator.h
  dispatcher/naive/MyopicMessageTaskTerminator.h
  dispatcher/greedy/BestFirstSequentialEntry.h
  Controller.h
  GreedyController.h
)
