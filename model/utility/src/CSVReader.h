#ifndef BPMNOS_CSVReader_H
#define BPMNOS_CSVReader_H

#include <string>
#include <vector>
#include <variant>

#include "model/utility/src/Number.h"
#include "model/utility/src/encode_collection.h"
#include "model/utility/src/encode_quoted_strings.h"

namespace BPMNOS {

class CSVReader {
public:
  using Row = std::vector< std::variant< std::string, BPMNOS::number > >;
  using Table = std::vector<Row>;

  explicit CSVReader(const std::string& instanceFileOrString);
  Table read();
  const std::string instanceFileOrString;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_LookupTable_H
