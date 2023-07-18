#ifndef BPMNOS_UnsetOperator_H
#define BPMNOS_UnsetOperator_H

#include "Attribute.h"

namespace BPMNOS {

class Operator;

class UnsetOperator {
public:
  UnsetOperator(Operator* base, Attribute* attribute);
  const Operator* base;
  Attribute* attribute;

  template <typename T>
  void execute(std::vector<std::optional<T> >& values) const {
    values[attribute->index] = std::nullopt;
  }
};

} // namespace BPMNOS

#endif // BPMNOS_UnsetOperator_H
