#ifndef BPMNOS_CSVReader_H
#define BPMNOS_CSVReader_H

#include <string>
#include <vector>
#include <variant>

#include "model/utility/src/Number.h"

namespace BPMNOS {

class CSVReader {
public:
  using Row = std::vector< std::variant< std::string, BPMNOS::number > >;
  using Table = std::vector<Row>;
  /// What a cell states: a number or truth value, a quoted string, an array or value with fields, or other text.
  enum class Type { NUMBER, STRING, OBJECT, TEXT };

  explicit CSVReader(const std::string& instanceFileOrString, const std::string& delimiters = ",;\t");
  /// Reads the table, every literal being replaced by the number encoding it.
  Table read();
  /// Returns what each cell of the table read last states, in the order of its rows and cells.
  const std::vector< std::vector<Type> >& types() const { return cellTypes; }
  const std::string instanceFileOrString;
  const std::string delimiters;
private:
  std::vector< std::vector<Type> > cellTypes;
};

} // namespace BPMNOS

#endif // BPMNOS_LookupTable_H
