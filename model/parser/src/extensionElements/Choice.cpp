#include "Choice.h"

using namespace BPMNOS::Model;

Choice::Choice(XML::bpmnos::tChoice* choice, AttributeMap& attributeMap)
  : element(choice)
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
    throw std::runtime_error("Choice: unsupported type 'string' for attribute '" + attribute->name + "'");
  }
}
