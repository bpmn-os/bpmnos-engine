#include "CSVReader.h"
#include <fstream>
#include <filesystem>
#include <strutil.h>

using namespace BPMNOS;

CSVReader::CSVReader(const std::string& filename)
  : filename(filename)
{
}

CSVReader::Table CSVReader::read() {
  Table table;
  std::ifstream file(filename);

  if (!file.is_open()) {
    throw std::runtime_error("CSVReader: Could not open file " + filename);
  }

  std::string line;
  while (std::getline(file, line)) {
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

  file.close();
  return table;
}
