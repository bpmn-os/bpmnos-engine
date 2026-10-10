#ifndef BPMNOS_LookupTable_H
#define BPMNOS_LookupTable_H

#include <string>
#include <unordered_map>
#include <variant>
#include <initializer_list>
#include <filesystem>
#include <optional>
#include "model/utility/src/Value.h"
#include "model/utility/src/Number.h"
#include "model/utility/src/vector_map.h"
#include "model/utility/src/CSVReader.h"
#include "model/bpmnos/src/extensionElements/Schema.h"

#include <functional>

namespace BPMNOS::Model {

/**
 * @brief Class generating a lookup table from a CSV file.
 *
 * A LookupTable loads a CSV file with header line and
 * allows to retrieve entries using a list of key-value pairs.
 * Should multiple entries match the given key-value pairs,
 * the first match is returned.
 *
 * The header the model declares states every column with its type, `From: string;To: string;Duration: decimal`,
 * in the syntax of attribute types. The keys, all columns but the last, are scalar, and the result, the last
 * column, may be an array or object, `Destinations: string[]`, which the lookup returns as the index of the
 * constant object its cell states. Every cell is checked against the type of its column when the table is
 * loaded: a string is quoted, a boolean, integer or decimal is a number or truth value, and an array or object is
 * a literal fitting the type.
 */
class LookupTable  {
public:
  /**
   * @brief Constructs a LookupTable object using data from a CSV file.
   *
   * This constructor initializes a LookupTable object using the data read from a CSV
   * file specified by the filename and folders. It first tries to read the file using 
   * the provided filename. If the file is not found in the current working directory, 
   * the function looks for the file in the list of folders provided in the folders parameter. 
   *
   * @param name The name of the lookup table.
   * @param source The name of the CSV file to read the data from.
   * @param header The expected columns as a semicolon separated list of names with their types,
   *        `name: type`. The names are validated against the actual column names of the CSV
   *        header line (trimmed and case insensitive); a mismatch throws.
   * @param folders The folder names in which lookup tables can be found.
   */
  LookupTable(const std::string& name, const std::string& source, const std::string& header, const std::vector<std::string>& folders);

  /**
   * @brief Constructs a LookupTable object from in-memory CSV content.
   *
   * @param name The name of the lookup table.
   * @param csvContent The CSV content (including the header line) to read the data from.
   * @param header The expected columns as a semicolon separated list of names with their types (see above).
   */
  LookupTable(const std::string& name, const std::string& csvContent, const std::string& header);
  const std::string name;
  const std::string header;
  const std::vector< std::pair<std::string, Schema> > columns; ///< The columns the header declares, with their types

  double at( const std::vector< double >& keys ) const;
  size_t size() const { return lookupMap.size(); }
protected:
  CSVReader openCsv(const std::string& filename, const std::vector<std::string>& folders);
  /// Validates the actual CSV header row against the expected column names in @ref header
  /// (trimmed and case insensitive). Throws std::runtime_error on any mismatch.
  void validateHeader(const std::string& sourceLabel, const CSVReader::Row& headerRow) const;
  /// Parses the columns the header declares, each of which must state its type.
  std::vector< std::pair<std::string, Schema> > parseColumns() const;
  void populate(const std::string& sourceLabel, CSVReader reader);
  BPMNOS::vector_map< std::vector< double >, double > lookupMap;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_LookupTable_H
