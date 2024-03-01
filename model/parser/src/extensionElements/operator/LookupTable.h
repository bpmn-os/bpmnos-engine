#ifndef BPMN_LookupTable_H
#define BPMN_LookupTable_H

#include <csv.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <initializer_list>
#include <filesystem>
#include "model/parser/src/extensionElements/Attribute.h"
#include "model/utility/src/Value.h"

namespace BPMNOS::Model {

/**
 * @brief Class generating a lookup table from a CSV file.
 *
 * A LookupTable loads a CSV file with header line and
 * allows to retrieve entries using a list of key-value pairs.
 * Should multiple entries match the given key-value pairs,
 * the first match is returned.
 */
class LookupTable  {
public:
  LookupTable() {};
  /**
   * @brief Constructs a LookupTable object using data from a CSV file.
   *
   * This constructor initializes a LookupTable object using the data read from a CSV
   * file specified by the filename and folders. It first tries to read the file using 
   * the provided filename. If the file is not found in the current working directory, 
   * the function looks for the file in the list of folders provided in the folders parameter. 
   *
   * @param filename The name of the CSV file to read the data from.
   */
  LookupTable(const std::string& filename);

  /**
   * @brief A vector of folders in which to search for lookup tables.
   **/ 
  static inline std::vector<std::string> folders = std::vector<std::string>();

  /**
   * @brief Performs a lookup operation in the LookupTable based on the given key and arguments.
   *
   * This function performs a lookup operation in the LookupTable using the provided key and arguments.
   * It searches for a row in the LookupTable's data that matches the given arguments.
   *
   * @param key The key used for the lookup operation.
   * @param arguments The arguments used to filter the rows during the lookup operation.
   *
   * @return An optional string containing the value found in the specified row and column if a matching row is found.
   *         If no matching row is found, std::nullopt is returned.
   */
  std::optional<std::string> lookup(const std::string& key, const std::unordered_map< std::string, Value > &arguments) const;

//  unsigned int size() { return data.size(); }
protected:
  csv::CSVReader openCsv(const std::string& filename);
  const csv::CSVRow* row(const std::unordered_map< std::string, Value > &arguments) const;
  std::vector<csv::CSVRow> data;
};

} // namespace bpmn::execution

#endif // BPMN_LookupTable_H
