#include "CSVReader.h"
#include "Keywords.h"
#include <sstream>
#include <fstream>
#include <filesystem>
#include "string_utility.h"
#include "InputEncoder.h"
#include <algorithm>
#include <cctype>

using namespace BPMNOS;

CSVReader::CSVReader(const std::string& instanceFileOrString, const std::string& delimiters)
  : instanceFileOrString(instanceFileOrString)
  , delimiters(delimiters)
{
}

CSVReader::Table CSVReader::read() {
  std::unique_ptr<std::istream> input;
    
  if (instanceFileOrString.contains("\n")) {
    // parameter contains linebreak, assume that it is the csv content
    input = std::make_unique<std::istringstream>(instanceFileOrString);
  }
  else {
    // parameter contains no linebreak, assume that it is a filename
    auto fileStream = std::make_unique<std::ifstream>(instanceFileOrString);
    if (!fileStream->is_open()) {
      throw std::runtime_error("CSVReader: Could not open file " + instanceFileOrString);
    }
    input = std::move(fileStream);
  }

  Table table;
  cellTypes.clear();

  std::string line;
  while (std::getline(*input, line)) {
    InputEncoder encoder( line );
    line = encoder.text();
    if ( BPMNOS::trim_copy(line).empty() ) continue; // skip empty lines
    // the cells are the stretches between delimiters, a literal having been replaced by the number encoding it,
    // so that a delimiter within a literal separates nothing
    Row row;
    std::vector<Type> types;
    // the line is split without its surrounding whitespace, which may hold a delimiter such as a tab
    size_t lineEnd = line.find_last_not_of(" \t\r\n\f\v") + 1;
    size_t begin = line.find_first_not_of(" \t\r\n\f\v");
    while ( begin <= lineEnd ) {
      size_t end = std::min(line.find_first_of(delimiters, begin), lineEnd);
      size_t first = begin;
      size_t last = end;
      while ( first < last && std::isspace((unsigned char)line[first]) ) ++first;
      while ( last > first && std::isspace((unsigned char)line[last - 1]) ) --last;
      std::string cell = line.substr(first, last - first);
      begin = end + 1;

      auto span = std::ranges::find_if(encoder.spans(), [first, last](auto& candidate) { return candidate.begin == first && candidate.end == last; });
      if ( span != encoder.spans().end() ) {
        // the cell states a literal, which the number encodes
        row.push_back((BPMNOS::number)std::stod(cell));
        types.push_back( span->object ? Type::OBJECT : Type::STRING );
      }
      else if ( !cell.empty() && (std::isdigit( cell[0] ) || cell[0] == '.' || cell[0] == '-') ) {
        // treat cell as number
        row.push_back((BPMNOS::number)std::stod(cell));
        types.push_back(Type::NUMBER);
      }
      else if ( cell == Keyword::True ) {
        row.push_back((BPMNOS::number)1);
        types.push_back(Type::NUMBER);
      }
      else if ( cell == Keyword::False ) {
        row.push_back((BPMNOS::number)0);
        types.push_back(Type::NUMBER);
      }
      else {
        types.push_back(Type::TEXT);
        // treat cell as string
        // Note: construct the alternative explicitly. The variant's converting
        // constructor would otherwise test std::string -> BPMNOS::number
        // convertibility, forcing instantiation of cnl's unconstrained
        // string constructor, which fails to compile under clang.
        row.emplace_back(std::in_place_type<std::string>, cell);
      }
    }
    table.push_back(row);
    cellTypes.push_back(std::move(types));
  }

  return table;
}
