#include "Decision.h"

using namespace BPMNOS::Model;

Decision::Decision(XML::bpmnos::tDecision* decision, AttributeMap& attributeMap)
  : element(decision)
  , attribute(attributeMap.at(element->attribute.value))
{
  attribute->isImmutable = false;

  if ( attribute->type == BOOLEAN ) {
    min = 0;
    max = 1;
  }
  else if ( attribute->type == INTEGER ) {
    assert( std::numeric_limits<BPMNOS::number>::min() < 0 );
    min = BPMNOS::to_number( (int)std::numeric_limits<BPMNOS::number>::min(), INTEGER);
    assert( std::numeric_limits<BPMNOS::number>::max() > 0 );
    max = BPMNOS::to_number( (int)std::numeric_limits<BPMNOS::number>::max(), INTEGER);
  }
  else if ( attribute->type == DECIMAL ) {
    min = std::numeric_limits<BPMNOS::number>::min();
    max = std::numeric_limits<BPMNOS::number>::max();
  }
  else {
    throw std::runtime_error("Decision: unsupported type 'string' for attribute '" + attribute->name + "'");
  }
}
