#include "LookupTable.h"
#include "model/utility/src/string_utility.h"
#include "model/utility/src/ObjectRegistry.h"
#include <ranges>
#include <iostream>
#include <algorithm>
#include <cctype>

using namespace BPMNOS::Model;

LookupTable::LookupTable(const std::string& name, const std::string& source, const std::string& header, const std::vector<std::string>& folders)
  : name(name)
  , header(header)
  , columns(parseColumns())
{
  populate(source, openCsv(source,folders));
}

LookupTable::LookupTable(const std::string& name, const std::string& csvContent, const std::string& header)
  : name(name)
  , header(header)
  , columns(parseColumns())
{
  populate(name, CSVReader(csvContent));
}

std::vector< std::pair<std::string, Schema> > LookupTable::parseColumns() const {
  std::vector< std::pair<std::string, Schema> > result;
  for ( auto& column : BPMNOS::split(header, ';') ) {
    auto colon = column.find(':');
    if ( colon == std::string::npos ) {
      throw std::runtime_error(std::format("LookupTable: column '{}' of table '{}' requires a type, 'name: type'", BPMNOS::trim_copy(column), name));
    }
    auto columnName = BPMNOS::trim_copy(column.substr(0, colon));
    Schema schema;
    try {
      schema = Schema::parse(BPMNOS::trim_copy(column.substr(colon + 1)));
    }
    catch ( const std::exception& error ) {
      throw std::runtime_error(std::format("LookupTable: illegal type of column '{}' of table '{}'.\n{}", columnName, name, error.what()));
    }
    result.emplace_back(columnName, std::move(schema));
  }
  if ( result.size() < 2 ) {
    throw std::runtime_error(std::format("LookupTable: table '{}' requires a key and a result column", name));
  }
  for ( size_t i = 0; i + 1 < result.size(); i++ ) {
    if ( !result[i].second.isScalar() ) {
      throw std::runtime_error(std::format("LookupTable: key column '{}' of table '{}' must have a scalar type", result[i].first, name));
    }
  }
  return result;
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
  std::vector<std::string> expected;
  for ( auto& [columnName, _] : columns ) {
    expected.push_back(columnName);
  }

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

void LookupTable::populate(const std::string& sourceLabel, CSVReader reader) {
  auto table = reader.read();
  auto& types = reader.types();
  if ( table.empty() ) {
    throw std::runtime_error(std::format("LookupTable: table '{}' with source '{}' is empty", name, sourceLabel));
  }
  // validate the CSV header line (index 0) against the expected column names
  validateHeader(sourceLabel, table[0]);
  // populate lookup map
  for (size_t j = 1; j < table.size(); j++) {   // assume a single header line at index 0
    auto& row = table[j];
    if ( row.size() != this->columns.size() ) {
      throw std::runtime_error(std::format("LookupTable: row {} of table '{}' has {} cells instead of {}", j, name, row.size(), this->columns.size()));
    }
    // every cell is checked against the type of its column
    for ( size_t i = 0; i < row.size(); i++ ) {
      auto& [columnName, schema] = this->columns[i];
      auto type = types[j][i];
      bool fits = schema.isScalar() ?
        ( schema.scalar.value() == BPMNOS::ValueType::STRING ? type == BPMNOS::CSVReader::Type::STRING : type == BPMNOS::CSVReader::Type::NUMBER ) :
        type == BPMNOS::CSVReader::Type::OBJECT;
      if ( !fits ) {
        throw std::runtime_error(std::format("LookupTable: cell of row {} in column '{}' of table '{}' is no {}", j, columnName, name, schema.stringify()));
      }
      if ( type == BPMNOS::CSVReader::Type::OBJECT ) {
        try {
          schema.conform( objectRegistry[(size_t)std::get<BPMNOS::number>(row[i])] );
        }
        catch ( const std::exception& error ) {
          throw std::runtime_error(std::format("LookupTable: cell of row {} in column '{}' of table '{}' is no {}.\n{}", j, columnName, name, schema.stringify(), error.what()));
        }
      }
    }
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
