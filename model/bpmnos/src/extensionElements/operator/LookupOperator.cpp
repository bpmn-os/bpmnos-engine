#include "LookupOperator.h"
#include "model/bpmnos/src/extensionElements/Operator.h"

using namespace BPMNOS::Model;

LookupOperator::LookupOperator(XML::bpmnos::tOperator* operator_, const AttributeRegistry& attributeRegistry)
  : Operator(operator_, attributeRegistry)
{
  try {
    parameterMap.at("source").get();
  }
  catch ( ... ){
    throw std::runtime_error("Lookup: required parameter 'source' not provided for operator '" + id + "'");
  }

  for ( auto &[parameterName,parameter] : parameterMap ) {
    if ( parameterName == "source" ) {
      if ( !parameter->value.has_value() || parameter->value.value().get().value != "file" ) {
        throw std::runtime_error("Lookup: unknown source of operator '" + id + "'");
      }
    }
    else if ( parameterName == "filename" ) {
      if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
        throw std::runtime_error("Lookup: filename of operator '" + id + "' must be given by value");
      }
      filename = parameter->value.value().get().value;
    }
    else if ( parameterName == "key" ) {
      if ( parameter->attribute.has_value() || !parameter->value.has_value() ) {
        throw std::runtime_error("Lookup: key of operator '" + id + "' must be given by value");
      }
      key = parameter->value.value().get().value;
    }
    else {
      if ( !parameter->attribute.has_value() || parameter->value.has_value() ) {
        throw std::runtime_error("Lookup: lookup argument of operator '" + id + "' must be given by attribute name");
      }
      lookups.push_back({ parameterName, &parameter->attribute->get() });
      inputs.insert( &parameter->attribute->get() );
    }
  }

  auto& filenameParameter = parameterMap["filename"];

  if ( filenameParameter->value.has_value() ) {
    filename = filenameParameter->value.value().get().value;
  }
  else {
    throw std::runtime_error("LookupOperator: filename missing for operator '" + id + "'");
  }

  // make sure lookup table is read and available;
  lookupTable = getLookupTable(filename);
  // make sure lookup map is populated;
  lookupMap = lookupTable->getLookupMap(lookups,{key,attribute});
}

template <typename DataType>
void LookupOperator::_apply(BPMNOS::Values& status, DataType& data, BPMNOS::Values& globals) const {
  std::vector< BPMNOS::number > inputs;
  for ( auto& [key,attribute] : lookups) {
    auto value = attributeRegistry.getValue(attribute, status, data, globals);
    if ( !value.has_value() ) {
      // set attribute to undefined because required lookup value is not given
      attributeRegistry.setValue( attribute, status, data, globals, std::nullopt );
      return;
    }
    inputs.push_back(value.value());
  }
  
  auto it = lookupMap->find( inputs );
  if ( it == lookupMap->end() ) {
    // set attribute to undefined because lookup value is not found
    attributeRegistry.setValue( attribute, status, data, globals, std::nullopt );
    return;
  }

  // set value to the value looked up
  attributeRegistry.setValue( attribute, status, data, globals, it->second );
}

template void LookupOperator::_apply<BPMNOS::Values>(BPMNOS::Values& status, BPMNOS::Values& data, BPMNOS::Values& globals) const;
template void LookupOperator::_apply<BPMNOS::SharedValues>(BPMNOS::Values& status, BPMNOS::SharedValues& data, BPMNOS::Values& globals) const;




