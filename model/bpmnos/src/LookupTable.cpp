#include "LookupTable.h"
#include <iostream>

using namespace BPMNOS::Model;

LookupTable::LookupTable(const std::string& name, const std::string& source)
  : name(name)
{
  createMap(source);
}

BPMNOS::CSVReader LookupTable::openCsv(const std::string& filename) {
  // First, try to open the file using the given filename.
  if (std::filesystem::exists(filename)) {
    return CSVReader(filename);
  }

  // If the file is not found with the given filename, try each folder in the list.
  for (const std::string& folder : folders) {
    std::filesystem::path fullPath = std::filesystem::path(folder) / filename;
    if (std::filesystem::exists(fullPath)) {
      return CSVReader(fullPath.string());
    }
  }

  // If the file is not found in any of the folders, throw an exception or handle the situation as needed.
  throw std::runtime_error("LookupTable: CSV file '" + filename + "'not found");
}

void LookupTable::createMap(const std::string& source) {
  BPMNOS::CSVReader reader = openCsv(source);
  CSVReader::Table table = reader.read();
  if ( table.empty() ) {
    throw std::runtime_error("LookupTable: table '" + source + "'is empty");
  }
  // assume a single header line
  // populate lookup map
  for (size_t i=1; i < table.size(); i++ ) {
//std::cerr << "row[" << i << "] = ";
    auto& row = table[i];

    std::vector< double > inputs;
    size_t columns = row.size();
    
    for ( size_t i = 0; i < columns - 1; i++ ) {
      auto& cell = row[i];
      if ( !std::holds_alternative<BPMNOS::number>(cell) ) {
        throw std::runtime_error("LookupTable: illegal input in table '" + source + "'");
      }
//std::cerr << (double)std::get<BPMNOS::number>(cell) << ", ";
      inputs.push_back( (double)std::get<BPMNOS::number>(cell) );
    }
    auto& cell = row[columns - 1];
    if ( !std::holds_alternative<BPMNOS::number>(cell) ) {
      std::visit([](auto&& value) { std::cerr <<  "Value: " << value << " "; }, cell);
      throw std::runtime_error("LookupTable: illegal output in table '" + source + "'");
    }
//std::cerr << "-> " << (double)std::get<BPMNOS::number>(cell) << std::endl;
    auto result = (double)std::get<BPMNOS::number>(cell);
    lookupMap.emplace( std::move(inputs), result );
  }
}

double LookupTable::at( const std::vector< double >& keys ) const {
  auto it = lookupMap.find(keys);
  if ( it != lookupMap.end() ) {
    return it->second;
  }
  throw std::runtime_error("LookupTable: keys not found in table.");
}
