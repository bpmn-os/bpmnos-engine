#ifndef BPMNOS_Unset_H
#define BPMNOS_Unset_H

#include "model/parser/src/Attribute.h"

namespace BPMNOS {

class Operator;

class Unset {
public:
  Unset(Operator* base, Attribute* attribute);
  const Operator* base;
  Attribute* attribute;

  template <typename T>
  void execute(std::vector<std::optional<T> >& values) const {
    values[attribute->index] = std::nullopt;
  }
};

} // namespace BPMNOS

#endif // BPMNOS_Unset_H
