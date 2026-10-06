list(APPEND execution_data_SOURCES
  Scenario.cpp
  DataProvider.cpp
  LegacyDataProvider.cpp
  InstanceDataReader.cpp
  StaticDataProvider.cpp
  ExpectedValueDataProvider.cpp
  DynamicDataProvider.cpp
  StochasticDataProvider.cpp
)

list(APPEND execution_data_HEADERS
  Scenario.h
  DataProvider.h
  LegacyDataProvider.h
  InstanceDataReader.h
  StaticDataProvider.h
  ExpectedValueDataProvider.h
  DynamicDataProvider.h
  StochasticDataProvider.h
)
