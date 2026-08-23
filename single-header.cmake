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

# PROLOGUE names headers the amalgamation depends on but does not contain. Quote-form includes are stripped
# below and angle-form ones stay where their source header had them, which for a type used by the first
# class in the file is too late; naming them here makes the result self-contained.
if(DEFINED PROLOGUE)
  string(REPLACE "\"" "" PROLOGUE "${PROLOGUE}")
  string(REPLACE " " ";" PROLOGUE_LIST "${PROLOGUE}")
  foreach(header ${PROLOGUE_LIST})
    file(APPEND "${OUTPUT}" "#include <${header}>\n")
  endforeach()
endif()

foreach(header ${HEADERS_LIST})
  file(READ "${header}" contents)
  # Remove includes using quotes (with the last being directly followed by linebreak)
  string(REGEX REPLACE "#include[ \t]+\"[^\"]*\"(\r?\n|\r)" "" contents "${contents}")
  file(APPEND "${OUTPUT}" "${contents}\n")
endforeach()
