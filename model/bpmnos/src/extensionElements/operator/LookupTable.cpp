#include "LookupTable.h"

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
    // determine delimiter from file
    auto delimiter = csv::guess_format(filename).delim;
    (delimiter == '\t') ? format.trim({' '}) : format.trim({' ', '\t'});
    csv::CSVReader reader = csv::CSVReader(filename, format);
    return reader;
  }

  // If the file is not found with the given filename, try each folder in the list.
  for (const std::string& folder : folders) {
    std::filesystem::path fullPath = std::filesystem::path(folder) / filename;
    if (std::filesystem::exists(fullPath)) {
      // determine delimiter from file
      auto delimiter = csv::guess_format(fullPath.string()).delim;
      (delimiter == '\t') ? format.trim({' '}) : format.trim({' ', '\t'});
      csv::CSVReader reader = csv::CSVReader(fullPath.string(), format);
      return reader;
    }
  }

  // If the file is not found in any of the folders, throw an exception or handle the situation as needed.
  throw std::runtime_error("CSV file not found.");
}

std::optional<std::string> LookupTable::lookup(const std::string& key, const std::unordered_map< std::string, Value > &arguments) const {
  const csv::CSVRow* matchingRow = row(arguments);
  if ( matchingRow ) {
    return (*matchingRow)[key].get<>();
  }
  return std::nullopt;
}

const csv::CSVRow* LookupTable::row(const std::unordered_map< std::string, Value > &arguments) const {
  for (const csv::CSVRow& row : data) {
    bool SAME = true;
    for ( auto &[columnName,columnValue] : arguments ) {
      if (std::holds_alternative<std::string>(columnValue)) {
        SAME = ( std::get<std::string>(columnValue) == row[columnName].get<std::string>() );
      }
      else if (std::holds_alternative<bool>(columnValue)) {
        SAME = ( std::get<bool>(columnValue) == (row[columnName].get<std::string>() == "true") );
      }
      else if (std::holds_alternative<int>(columnValue)) {
        SAME = ( std::get<int>(columnValue) == BPMNOS::stoi(row[columnName].get<std::string>()) );
      }
      else if (std::holds_alternative<double>(columnValue)) {
        SAME = ( std::abs( std::get<double>(columnValue) - BPMNOS::stod(row[columnName].get<std::string>()) )
                  <= std::numeric_limits<double>::epsilon() 
               );
      }

      if (!SAME) {
        // no need to continue with next arguments because row is not matching
        break;
      }
    }

    if ( SAME == true) {
      return &row;
    }
  }
  return nullptr;
}
