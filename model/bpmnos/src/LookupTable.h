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
#include "model/utility/src/encode_collection.h"
#include "model/utility/src/encode_quoted_strings.h"
#include "model/utility/src/CSVReader.h"

#include <functional>

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
  LookupTable(const std::string& name, const std::string& source);
  const std::string name;

  /**
   * @brief A vector of folders in which to search for lookup tables.
   **/ 
  static inline std::vector<std::string> folders = std::vector<std::string>();

  double at( const std::vector< double >& keys ) const;
  size_t size() const { return lookupMap.size(); }
protected:
  CSVReader openCsv(const std::string& filename);
  void createMap(const std::string& source);
  BPMNOS::vector_map< std::vector< double >, double > lookupMap;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_LookupTable_H
