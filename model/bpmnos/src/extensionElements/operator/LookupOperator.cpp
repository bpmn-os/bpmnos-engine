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
  table = getLookupTable(filename);
}

template <typename DataType>
void LookupOperator::_apply(BPMNOS::Values& status, DataType& data) const {
  std::unordered_map< std::string, Value > arguments;
  for ( auto& [name,lookupAttribute] : lookups) {
    auto value = attributeRegistry.getValue(lookupAttribute, status, data);
    if ( !value.has_value() ) {
      // set attribute to undefined because required lookup value is not given
      attributeRegistry.setValue( attribute, status, data, std::nullopt );
      return;
    }

    arguments[name] = to_string(value.value(),lookupAttribute->type);
  }

  std::optional<std::string> value = table->lookup(key, arguments);
  if ( value.has_value() ) {
    // set value to the value looked up
    attributeRegistry.setValue( attribute, status, data, to_number( value.value(), attribute->type ) );
  }
  else {
    // set value to undefined if no attribute with value is given and no explicit value is given
    attributeRegistry.setValue( attribute, status, data, std::nullopt );
  }
}

template void LookupOperator::_apply<BPMNOS::Values>(BPMNOS::Values& status, BPMNOS::Values& data) const;
template void LookupOperator::_apply<BPMNOS::SharedValues>(BPMNOS::Values& status, BPMNOS::SharedValues& data) const;




