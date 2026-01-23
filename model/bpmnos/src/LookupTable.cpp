#include "LookupTable.h"
#include <ranges>
#include <iostream>

using namespace BPMNOS::Model;

LookupTable::LookupTable(const std::string& name, const std::string& source, const std::vector<std::string>& folders)
  : name(name)
{
  createMap(source,folders);
}

BPMNOS::CSVReader LookupTable::openCsv(const std::string& filename, const std::vector<std::string>& folders) {
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

  // If the file is not found in any of the folders, throw an exception with all searched locations
  std::string errorMsg = "LookupTable: CSV file '" + filename + "' not found in:\n" + std::filesystem::current_path().string();
  for (const std::string& folder : folders) {
    errorMsg += "\n" + std::filesystem::absolute(folder).string();
  }
  throw std::runtime_error(errorMsg);
}

void LookupTable::createMap(const std::string& source, const std::vector<std::string>& folders) {
  BPMNOS::CSVReader reader = openCsv(source,folders);
  CSVReader::Table table = reader.read();
  if ( table.empty() ) {
    throw std::runtime_error("LookupTable: table '" + source + "' is empty");
  }
  // populate lookup map
  for (auto& row : table | std::views::drop(1)) {   // assume a single header line
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
