#include "LookupTable.h"
#include "model/utility/src/string_utility.h"
#include <ranges>
#include <iostream>
#include <algorithm>
#include <cctype>

using namespace BPMNOS::Model;

LookupTable::LookupTable(const std::string& name, const std::string& source, const std::string& header, const std::vector<std::string>& folders)
  : name(name)
  , header(header)
{
  populate(source, openCsv(source,folders).read());
}

LookupTable::LookupTable(const std::string& name, const std::string& csvContent, const std::string& header)
  : name(name)
  , header(header)
{
  populate(name, CSVReader(csvContent).read());
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

void LookupTable::validateHeader(const std::string& sourceLabel, const CSVReader::Row& headerRow) const {
  // Normalise a column name for comparison: trim surrounding whitespace and fold to lower case.
  auto normalise = [](std::string s) {
    BPMNOS::trim(s);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
  };

  // Render a header cell as its column name (CSVReader may have parsed a numeric-looking name as a number).
  auto cellName = [](const CSVReader::Row::value_type& cell) -> std::string {
    if ( std::holds_alternative<std::string>(cell) ) {
      return std::get<std::string>(cell);
    }
    return std::format("{}", (double)std::get<BPMNOS::number>(cell));
  };

  // Expected column names come from the semicolon separated header attribute.
  auto expected = BPMNOS::split(header, ';');

  if ( expected.size() != headerRow.size() ) {
    throw std::runtime_error(std::format(
      "LookupTable: table '{}' with source '{}' expects {} column(s) ('{}') but the CSV header has {}",
      name, sourceLabel, expected.size(), header, headerRow.size()));
  }

  for ( size_t i = 0; i < expected.size(); i++ ) {
    auto actual = cellName(headerRow[i]);
    if ( normalise(expected[i]) != normalise(actual) ) {
      throw std::runtime_error(
        std::format("LookupTable: table '{}' with source '{}' expects column {} to be '{}' but the CSV header has '{}'", name, sourceLabel, i + 1, BPMNOS::trim_copy(expected[i]), BPMNOS::trim_copy(actual))
      );
    }
  }
}

void LookupTable::populate(const std::string& sourceLabel, CSVReader::Table table) {
  if ( table.empty() ) {
    throw std::runtime_error(std::format("LookupTable: table '{}' with source '{}' is empty", name, sourceLabel));
  }
  // validate the CSV header line (index 0) against the expected column names
  validateHeader(sourceLabel, table[0]);
  // populate lookup map
  for (size_t j = 1; j < table.size(); j++) {   // assume a single header line at index 0
    auto& row = table[j];
    std::vector< double > inputs;
    size_t columns = row.size();

    for ( size_t i = 0; i < columns - 1; i++ ) {
      auto& cell = row[i];
      if ( !std::holds_alternative<BPMNOS::number>(cell) ) {
        throw std::runtime_error(std::format("LookupTable: illegal input in table '{}' at row {}, column {}", name, j, i));
      }
//std::cerr << (double)std::get<BPMNOS::number>(cell) << ", ";
      inputs.push_back( (double)std::get<BPMNOS::number>(cell) );
    }
    auto& cell = row[columns - 1];
    if ( !std::holds_alternative<BPMNOS::number>(cell) ) {
      std::visit([](auto&& value) { std::cerr <<  "Value: " << value << " "; }, cell);
      throw std::runtime_error(std::format("LookupTable: illegal output in table '{}' at row {}, column {}", name, j, columns - 1));
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
  throw std::runtime_error(std::format("LookupTable: keys not found in table '{}'", name));
}
