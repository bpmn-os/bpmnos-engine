#include "LookupTable.h"
#include "model/utility/src/getDelimiter.h"

using namespace BPMNOS::Model;

LookupTable::LookupTable(const std::string& filename) {
  csv::CSVReader reader = openCsv(filename);
  for (auto &row : reader) {
    data.push_back(std::move(row));
  }
}

csv::CSVReader LookupTable::openCsv(const std::string& filename) {
  csv::CSVFormat format;

  // First, try to open the file using the given filename.
  if (std::filesystem::exists(filename)) {
    format.delimiter( { getDelimiter(filename) } );
    (format.get_delim() == '\t') ? format.trim({' '}) : format.trim({' ', '\t'});
    csv::CSVReader reader = csv::CSVReader(filename, format);
    return reader;
  }

  // If the file is not found with the given filename, try each folder in the list.
  for (const std::string& folder : folders) {
    std::filesystem::path fullPath = std::filesystem::path(folder) / filename;
    if (std::filesystem::exists(fullPath)) {
      format.delimiter( { getDelimiter(fullPath.string()) } );
      (format.get_delim() == '\t') ? format.trim({' '}) : format.trim({' ', '\t'});
      csv::CSVReader reader = csv::CSVReader(fullPath.string(), format);
      return reader;
    }
  }

  // If the file is not found in any of the folders, throw an exception or handle the situation as needed.
  throw std::runtime_error("CSV file not found.");
}

BPMNOS::vector_map< std::vector< BPMNOS::number >, BPMNOS::number >* LookupTable::getLookupMap(const std::vector< std::pair< std::string, Attribute*> >& keys, const std::pair< std::string, Attribute*>& value) {
  std::vector<std::string> headers;
  for ( auto& [ header, _] : keys ) {
    headers.push_back(header);
  }
  headers.push_back(value.first); // add output header
  
  if ( lookupMaps.find(headers) == lookupMaps.end() ) {
    // create and populate lookup map
    BPMNOS::vector_map< std::vector< BPMNOS::number >, BPMNOS::number > lookupMap;
    
    for ( auto& row : data ) {
      std::vector< BPMNOS::number > inputs;
      for ( auto& [key, attribute] : keys ) {
        inputs.push_back( BPMNOS::to_number( row[key].get<std::string>(), attribute->type) );
      }
      lookupMap.emplace( inputs, BPMNOS::to_number( row[value.first].get<std::string>(), value.second->type) );
    }
    
    lookupMaps.emplace( headers, std::move(lookupMap) );
  }
  
  return &lookupMaps.at(headers);
}

