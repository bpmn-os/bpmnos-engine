list(APPEND model_utility_SOURCES
  Number.cpp
  StringRegistry.cpp
  CollectionRegistry.cpp
  CSVReader.cpp
  RandomDistributionFactory.cpp
)

list(APPEND model_utility_HEADERS
  Keywords.h
  Value.h
  Number.h
  StringRegistry.h
  CollectionRegistry.h
  CSVReader.h
  RandomDistributionFactory.h
  getDelimiter.h
  tuple_map.h
  vector_map.h
  encode_quoted_strings.h
  encode_collection.h
)
