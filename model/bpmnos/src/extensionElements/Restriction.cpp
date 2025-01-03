#include "Restriction.h"

using namespace BPMNOS::Model;

Restriction::Restriction(XML::bpmnos::tRestriction* restriction, const AttributeRegistry& attributeRegistry)
  : element(restriction)
  , id(restriction->id.value.value)
  , expression(Expression(restriction->expression.value.value,attributeRegistry))
  , scope(Scope::FULL)
{
  if ( restriction->scope.has_value() ) {
    if ( restriction->scope->get().value.value == "entry" ) {
      scope = Scope::ENTRY;
    }
    else if ( restriction->scope->get().value.value == "exit" ) {
      scope = Scope::EXIT;
    }
  }  
}

template <typename DataType>
bool Restriction::isSatisfied(const BPMNOS::Values& status, const DataType& data, const BPMNOS::Values& globals) const {
  auto feasible = expression.execute(status,data,globals);
  return feasible.has_value() && feasible.value();
}

template bool Restriction::isSatisfied<BPMNOS::Values>(const BPMNOS::Values& status, const BPMNOS::Values& data, const BPMNOS::Values& globals) const;
template bool Restriction::isSatisfied<BPMNOS::SharedValues>(const BPMNOS::Values& status, const BPMNOS::SharedValues& data, const BPMNOS::Values& globals) const;
