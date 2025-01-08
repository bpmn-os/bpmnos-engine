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

  explicit CSVReader(const std::string& filename);
  Table read();

private:
  const std::string filename;
};

} // namespace BPMNOS::Model

#endif // BPMNOS_LookupTable_H

/*
int main() {
    try {
        CSVReader reader("example.csv");
        CSVReader::Table table = reader.read();

        for (const auto& row : table) {
            for (const auto& cell : row) {
                std::visit([](auto&& value) { std::cout << value << " "; }, cell);
            }
            std::cout << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    }

    return 0;
}
*/
