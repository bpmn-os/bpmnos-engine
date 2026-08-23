# Usage:
# cmake -DHEADERS="header1.h;header2.h" -DOUTPUT="/path/to/single-header.h" -P single-header.cmake

if(NOT DEFINED HEADERS)
  message(FATAL_ERROR "HEADERS variable not defined")
endif()

if(NOT DEFINED OUTPUT)
  message(FATAL_ERROR "OUTPUT variable not defined")
endif()

# Split the HEADERS string into a list
string(REPLACE "\"" "" HEADERS "${HEADERS}")
string(REPLACE " " ";" HEADERS_LIST "${HEADERS}")

# The output path is given rather than derived from this script's location, which tied the generated
# header to lib/ in the checkout no matter what the caller asked for.
message("-- Create single header file ${OUTPUT}")

file(WRITE "${OUTPUT}" "// Automatically generated single header file\n")

foreach(header ${HEADERS_LIST})
  file(READ "${header}" contents)
  # Remove includes using quotes (with the last being directly followed by linebreak)
  string(REGEX REPLACE "#include[ \t]+\"[^\"]*\"(\r?\n|\r)" "" contents "${contents}")
  file(APPEND "${OUTPUT}" "${contents}\n")
endforeach()
