#ifndef BPMNOS_Unset_H
#define BPMNOS_Unset_H

#include "model/parser/src/extensionElements/Attribute.h"
#include "model/utility/src/Number.h"

namespace BPMNOS {

class Operator;

class Unset {
public:
  Unset(Operator* base, Attribute* attribute);
  const Operator* base;
  Attribute* attribute;

/**
 * @brief Sets a status attribute as undefined.
 *
 * @param status The status values to be updated.
 */
  void execute(Values& status) const;
};

} // namespace BPMNOS

#endif // BPMNOS_Unset_H
