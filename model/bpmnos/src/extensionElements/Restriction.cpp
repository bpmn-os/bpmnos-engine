#include "Restriction.h"

using namespace BPMNOS::Model;

Restriction::Restriction(XML::bpmnos::tRestriction* restriction, AttributeMap& statusAttributes)
  : element(restriction)
  , id(restriction->id.value.value)
  , expression(Expression::create( &restriction->getRequiredChild<XML::bpmnos::tParameter>(), statusAttributes))
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

bool Restriction::isSatisfied(const Values& status) const {
  auto feasible = expression->execute(status);
  return feasible.has_value() && feasible.value();
}  


