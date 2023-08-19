#ifndef BPMNOS_Set_H
#define BPMNOS_Set_H

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/parser/src/extensionElements/Parameter.h"
#include "model/utility/src/Numeric.h"
#include "model/utility/src/StringRegistry.h"

namespace BPMNOS {

class Operator;

class Set {
public:
  Set(Operator* base, Attribute* attribute);
  const Operator* base;
  Attribute* attribute;
  Parameter* parameter;

  template <typename T>
  void execute(std::vector<std::optional<T> >& values) const {
    if ( parameter->attribute.has_value() && values[parameter->attribute->get().index].has_value() ) {
      // set value to value of given attribute (if not null)
      values[attribute->index] = values[parameter->attribute->get().index];
    }
    else if ( parameter->value.has_value() ) {
      // set value to given value
      switch ( attribute->type ) {
        case Attribute::Type::STRING :
          values[attribute->index] = stringRegistry((std::string)parameter->value->get());
          break;
        case Attribute::Type::BOOLEAN :
          values[attribute->index] = (bool)parameter->value->get();
          break;
        case Attribute::Type::INTEGER :
          values[attribute->index] = (int)parameter->value->get();
          break;
        case Attribute::Type::DECIMAL :
          values[attribute->index] = numeric<T>((double)parameter->value->get());
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

#endif // BPMNOS_Set_H
