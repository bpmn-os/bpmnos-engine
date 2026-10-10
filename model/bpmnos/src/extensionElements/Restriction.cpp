#include "Restriction.h"
#include "model/utility/src/InputEncoder.h"

using namespace BPMNOS::Model;

Restriction::Restriction(XML::bpmnos::tRestriction* restriction, const AttributeRegistry& attributeRegistry)
  : element(restriction)
  , id(restriction->id.value.value)
  , expression(Expression(InputEncoder(restriction->expression.value.value),attributeRegistry))
  , scope(Scope::FULL)
{
  if ( expression.type == Expression::Type::ASSIGN || expression.type == Expression::Type::UNASSIGN ) {
    throw std::runtime_error("Restriction: illegal restriction '" + expression.expression + "'");
  }
  if ( restriction->scope.has_value() ) {
    if ( restriction->scope->get().value.value == "entry" ) {
      scope = Scope::ENTRY;
    }
    else if ( restriction->scope->get().value.value == "completion" ) {
      scope = Scope::COMPLETION;
    }
    else if ( restriction->scope->get().value.value == "exit" ) {
      scope = Scope::EXIT;
    }
  }  
}

template <typename DataType>
bool Restriction::isSatisfied(const BPMNOS::Status& status, const DataType& data) const {
  auto feasible = expression.execute(status,data);
  return feasible.has_value() && (bool)feasible.value();
}

template bool Restriction::isSatisfied<BPMNOS::Data>(const BPMNOS::Status& status, const BPMNOS::Data& data) const;
template bool Restriction::isSatisfied<BPMNOS::SharedData>(const BPMNOS::Status& status, const BPMNOS::SharedData& data) const;
