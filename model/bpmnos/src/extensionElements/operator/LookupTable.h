#ifndef BPMNOS_LookupTable_H
#define BPMNOS_LookupTable_H

#include <csv.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <initializer_list>
#include <filesystem>
#include "model/bpmnos/src/extensionElements/Attribute.h"
#include "model/utility/src/Value.h"
#include "model/utility/src/vector_map.h"

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
   * @brief Container allowing to obtain lookup map for each header combination.
   **/
  BPMNOS::vector_map< std::vector<std::string>, BPMNOS::vector_map< std::vector< BPMNOS::number >, BPMNOS::number > > lookupMaps;
  /**
   * @brief Method returning lookup map for a given header combination.
   **/
  BPMNOS::vector_map< std::vector< BPMNOS::number >, BPMNOS::number >* getLookupMap(const std::vector< std::pair< std::string, Attribute*> >& keys, const std::pair< std::string, Attribute*>& value);


//  unsigned int size() { return data.size(); }
protected:
  csv::CSVReader openCsv(const std::string& filename);
  std::vector<csv::CSVRow> data;
};

} // namespace bpmn::execution

#endif // BPMNOS_LookupTable_H
