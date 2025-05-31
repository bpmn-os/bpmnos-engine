# Usage:
# cmake -DHEADERS="header1.h;header2.h" -DFILENAME="single-header.h" -P single-header.cmake

if(NOT DEFINED HEADERS)
  message(FATAL_ERROR "HEADERS variable not defined")
endif()

if(NOT DEFINED FILENAME)
  message(FATAL_ERROR "FILENAME variable not defined")
endif()

# Split the HEADERS string into a list
string(REPLACE "\"" "" HEADERS "${HEADERS}")
string(REPLACE " " ";" HEADERS_LIST "${HEADERS}")

get_filename_component(SCRIPT_DIR "${CMAKE_CURRENT_LIST_FILE}" DIRECTORY)
message("-- Create single header file ${SCRIPT_DIR}/lib/${FILENAME}")

file(WRITE "${SCRIPT_DIR}/lib/${FILENAME}" "// Automatically generated single header file\n")

foreach(header ${HEADERS_LIST})
  file(READ "${header}" contents)
  # Remove includes using quotes (with the last being directly followed by linebreak)
  string(REGEX REPLACE "#include[ \t]+\"[^\"]*\"(\r?\n|\r)" "" contents "${contents}")
  file(APPEND "${SCRIPT_DIR}/lib/${FILENAME}" "${contents}\n")
endforeach()
