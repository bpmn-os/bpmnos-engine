#include "CSVReader.h"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <strutil.h>

using namespace BPMNOS;

CSVReader::CSVReader(const std::string& instanceFileOrString)
  : instanceFileOrString(instanceFileOrString)
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

  std::string line;
  while (std::getline(*input, line)) {
    line = encodeCollection( encodeQuotedStrings( line ) );
    strutil::trim(line);
//std::cerr << "Line: " << line << std::endl;
    if ( line.empty() ) continue; // skip empty lines
    auto cells = strutil::split_any( line, ",;\t" );
    Row row;
    for ( auto cell : cells ) {
      strutil::trim(cell);
      if ( !cell.empty() && (std::isdigit( cell[0] ) || cell[0] == '.' || cell[0] == '-') ) {
        // treat cell as number
        row.push_back((BPMNOS::number)std::stod(cell));
      }
      else {
        // treat cell as string
        row.push_back(cell);
      }
    }
    table.push_back(row);
  }

  return table;
}
