#include "Input.h"
#include "model/utility/src/CSVReader.h"
#include "model/utility/src/ObjectRegistry.h"

using namespace BPMNOS::Model;

Input::Input(std::string name, Type type, Schema schema, const std::string& content)
  : name(std::move(name))
  , type(type)
  , schema(std::move(schema))
{
  try {
    if ( this->type == Type::MATRIX ) {
      object = readMatrix(content);
    }
    else if ( this->type == Type::OBJECT ) {
      object = this->schema.fromJSON(content);
    }
  }
  catch ( const std::exception& error ) {
    throw std::runtime_error("Input: illegal content of input '" + this->name + "'.\n" + error.what());
  }
}

std::shared_ptr<const BPMNOS::Object> Input::readMatrix(const std::string& content) const {
  auto array = std::get_if<BPMNOS::ArrayType>(&schema);
  auto row = array ? std::get_if<BPMNOS::ArrayType>(array->type.get()) : nullptr;
  if ( !row || std::holds_alternative<BPMNOS::ArrayType>(*row->type) ) {
    throw std::runtime_error("Input: matrix '" + name + "' requires a schema with two dimensions, not '" + schema.stringify() + "'");
  }
  // the content is read as content, which a single line without a line break would not be
  BPMNOS::CSVReader reader( content.ends_with('\n') ? content : content + "\n" );
  auto table = reader.read();
  auto& types = reader.types();

  // the element type of the matrix, of which every cell holds one value or literal
  auto& base = *row->type;
  auto valueType = std::get_if<BPMNOS::ValueType>(&base);
  size_t columns = table.empty() ? 0 : table.front().size();
  BPMNOS::Object::Layout cellLayout;
  std::vector< std::shared_ptr<const BPMNOS::Object> > cells;
  for ( size_t i = 0; i < table.size(); i++ ) {
    if ( table[i].size() != columns ) {
      throw std::runtime_error("Input: row " + std::to_string(i + 1) + " of matrix '" + name + "' has " + std::to_string(table[i].size()) + " cells instead of " + std::to_string(columns));
    }
    for ( size_t j = 0; j < columns; j++ ) {
      auto type = types[i][j];
      auto where = "cell " + std::to_string(j + 1) + " of row " + std::to_string(i + 1) + " of matrix '" + name + "'";
      if ( valueType ) {
        bool fits = ( *valueType == BPMNOS::ValueType::STRING ? type == BPMNOS::CSVReader::Type::STRING : type == BPMNOS::CSVReader::Type::NUMBER );
        if ( !fits ) {
          throw std::runtime_error("Input: " + where + " is no " + base.stringify());
        }
        continue;
      }
      if ( type != BPMNOS::CSVReader::Type::OBJECT ) {
        throw std::runtime_error("Input: " + where + " is no " + base.stringify());
      }
      auto index = (size_t)std::get<BPMNOS::number>(table[i][j]);
      auto& cellSchema = objectRegistry.schema(index);
      std::shared_ptr<const BPMNOS::Object> cell = base.supports(cellSchema) ? objectRegistry[index] : std::make_shared<const BPMNOS::Object>(base, *objectRegistry[index], cellSchema);
      if ( cells.empty() ) {
        cellLayout = *cell->layout;
      }
      else if ( !( *cell->layout == cellLayout ) ) {
        throw std::runtime_error("Input: " + where + " differs in its lengths from the first cell");
      }
      cells.push_back(std::move(cell));
    }
  }

  // the cells stand one after the other in the order of the rows
  auto matrix = std::make_shared<BPMNOS::Object>();
  BPMNOS::Object::Layout layout;
  if ( valueType ) {
    layout.scalar = *valueType;
  }
  else {
    layout = cellLayout;
    layout.fixed.clear();
  }
  layout.dimensions = { table.size(), columns };
  matrix->layout = std::make_shared<const BPMNOS::Object::Layout>(layout);
  matrix->values.reserve( layout.size() );
  for ( size_t i = 0; i < table.size(); i++ ) {
    for ( size_t j = 0; j < columns; j++ ) {
      if ( valueType ) {
        matrix->values.push_back( std::get<BPMNOS::number>(table[i][j]) );
      }
      else {
        auto& values = cells[i * columns + j]->values;
        matrix->values.insert( matrix->values.end(), values.begin(), values.end() );
      }
    }
  }
  // the cells are of the element type of the matrix, whose rows and columns are open until fitted to the schema:
  // fixed dimensions are padded, open ones take the size of the matrix
  Schema matrixSchema( ArrayType{ std::make_shared<const Schema>( ArrayType{ row->type, std::nullopt } ), std::nullopt } );
  return schema.supports(matrixSchema) ? matrix : std::make_shared<const BPMNOS::Object>(schema, *matrix, matrixSchema);
}
