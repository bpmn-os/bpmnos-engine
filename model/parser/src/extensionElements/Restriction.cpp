#include "Restriction.h"

using namespace BPMNOS::Model;

Restriction::Restriction(XML::bpmnos::tRestriction* restriction, AttributeMap& attributeMap)
  : element(restriction)
  , id(restriction->id.value.value)
  , expression(Expression::create( &restriction->getRequiredChild<XML::bpmnos::tParameter>(), attributeMap))
{
}

bool Restriction::isSatisfied(const Values& status) const {
  auto feasible = expression->execute(status);
  return feasible.has_value() && feasible.value();
}  


