#ifndef BPMNOS_Lookup_H
#define BPMNOS_Lookup_H

#include "model/parser/src/Attribute.h"
#include "model/parser/src/Parameter.h"
#include "model/utility/src/Numeric.h"
#include "model/utility/src/StringRegistry.h"
#include "LookupTable.h"

namespace BPMNOS {

class Operator;

class Lookup {
public:
  Lookup(Operator* base, Attribute* attribute);
  const Operator* base;
  Attribute* attribute;

  std::string filename;
  std::string key;
  std::vector< std::pair< std::string, Attribute*> > lookups;

  LookupTable* table;


  static inline std::unordered_map< std::string, LookupTable > lookupTable = {};
  static inline LookupTable* getLookupTable(const std::string& filename) {
    if ( lookupTable.find(filename) == lookupTable.end() ) {
      lookupTable[filename] = LookupTable(filename);
    }
    return &lookupTable[filename];
  };

  template <typename T>
  void execute(std::vector<std::optional<T> >& values) {

    Arguments arguments;
    for ( auto& [name,lookupAttribute] : lookups) {
      if ( !values[lookupAttribute->index].has_value() ) {
        // set attribute to undefined because required lookup value is not given
        values[attribute->index] = std::nullopt;
        return;
      }

      switch ( lookupAttribute->type ) {
        case Attribute::Type::STRING :
          arguments.push_back({ name, stringRegistry[ values[lookupAttribute->index] ] } );
          break;
        case Attribute::Type::BOOLEAN :
          arguments.push_back({ name, (bool)values[lookupAttribute->index] } );
          break;
        case Attribute::Type::INTEGER :
          arguments.push_back({ name, (int)values[lookupAttribute->index] } );
          break;
        case Attribute::Type::DECIMAL :
          arguments.push_back({ name, (double)values[lookupAttribute->index] } );
          break;
      }   
    }
    std::optional<std::string> value = table->lookup(key, arguments);

    if ( value.has_value() ) {
      // set value to given value
      switch ( attribute->type ) {
        case Attribute::Type::STRING :
          values[attribute->index] = numeric<T>(stringRegistry(*value));
          break;
        case Attribute::Type::BOOLEAN :
          values[attribute->index] = numeric<T>( (*value == "true") );
          break;
        case Attribute::Type::INTEGER :
          values[attribute->index] = numeric<T>( std::stoi(*value) );
          break;
        case Attribute::Type::DECIMAL :
          values[attribute->index] = numeric<T>( std::stod(*value) );
          break;
      }
    }
    else {
      // set value to undefined if no attribute with value is given and no explicit value is given
      values[attribute->index] = std::nullopt;
    }
  }
};

} // namespace BPMNOS

#endif // BPMNOS_Lookup_H
